"""프로젝트 전용 Serena의 다중 세션·재연결·심볼 탐색을 검증한다. Serena Python 환경에서 실행한다."""

import argparse
import asyncio
import json
from contextlib import AsyncExitStack
from pathlib import Path

from mcp import ClientSession
from mcp.client.streamable_http import streamable_http_client


def require(condition, message):
    if not condition:
        raise AssertionError(message)


async def connect(stack, url):
    read, write, get_session_id = await stack.enter_async_context(streamable_http_client(url))
    session = await stack.enter_async_context(ClientSession(read, write))
    await session.initialize()
    return session, get_session_id()


async def check_project(session, expected):
    result = await session.call_tool("get_current_config", {})
    require(not result.isError, "활성 프로젝트 조회 실패")
    output = "\n".join(item.text for item in result.content if item.type == "text")
    require(f"Active project: {expected}" in output, f"활성 프로젝트 불일치: {output}")
    return output


async def main(args):
    async with AsyncExitStack() as stack:
        first, first_id = await connect(stack, args.url)
        second, second_id = await connect(stack, args.url)
        require(first_id and second_id and first_id != second_id, "독립 MCP 세션이 생성되지 않았습니다")
        for session in (first, second):
            tools = {tool.name for tool in (await session.list_tools()).tools}
            require("activate_project" not in tools, "공유 서버에 프로젝트 전환 도구가 노출되었습니다")
            require("get_symbols_overview" in tools, "심볼 도구가 없습니다")
        before, _ = await asyncio.gather(check_project(first, args.project_name), check_project(second, args.project_name))
        denied = await second.call_tool("activate_project", {"project": str(Path(args.project_root).parent)})
        require(denied.isError, "제외된 프로젝트 전환 호출이 거부되지 않았습니다")
        after = await check_project(first, args.project_name)
        require(before == after, "다른 세션의 요청으로 서버 설정이 바뀌었습니다")
        if args.source:
            symbols = await first.call_tool("get_symbols_overview", {"relative_path": args.source, "depth": 1, "max_answer_chars": 16000})
            require(not symbols.isError, "HTTP 심볼 조회 실패")
            output = "\n".join(item.text for item in symbols.content if item.type == "text")
            require("CServer" in output, f"기대 심볼이 없습니다: {output}")
    # 두 세션을 모두 종료한 뒤에도 같은 서버의 활성 프로젝트가 유지된다.
    async with AsyncExitStack() as stack:
        reconnected, _ = await connect(stack, args.url)
        await check_project(reconnected, args.project_name)
        if args.source:
            result = await reconnected.call_tool("get_symbols_overview", {"relative_path": args.source, "depth": 1, "max_answer_chars": 16000})
            require(not result.isError and any("CServer" in item.text for item in result.content if item.type == "text"), "재연결 후 clangd 심볼 조회 실패")
    print(json.dumps({"ok": True, "endpoint": args.url, "project": args.project_name, "independent_sessions": 2, "project_switch_rejected": True, "reconnected": True, "symbols_checked": bool(args.source)}, ensure_ascii=False))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="http://127.0.0.1:9120/mcp")
    parser.add_argument("--project-name", default="war_of_dimension")
    parser.add_argument("--project-root", default=str(Path(__file__).resolve().parent.parent))
    parser.add_argument("--source", default="Server/Game_Server/CServer.h")
    asyncio.run(main(parser.parse_args()))
