"""Serena 1.2.0의 프로젝트/clangd 수명을 MCP 연결이 아닌 HTTP 서버 수명에 맞춘다."""

import argparse
import logging
import multiprocessing
from contextlib import asynccontextmanager

from serena.mcp import SerenaMCPFactory


class ProjectHttpFactory(SerenaMCPFactory):
    @asynccontextmanager
    async def server_lifespan(self, mcp_server):
        # FastMCP는 MCP 세션마다 이 함수를 호출한다. 세션 종료 시 agent를 종료하지 않는다.
        yield


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", required=True)
    parser.add_argument("--context", required=True)
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)
    factory = ProjectHttpFactory(context=args.context, project=args.project)
    try:
        server = factory.create_mcp_server(
            host="127.0.0.1",
            port=args.port,
            enable_web_dashboard=False,
            open_web_dashboard=False,
            enable_gui_log_window=False,
        )
        # Serena의 백그라운드 초기화가 실패한 상태를 HTTP 준비 완료로 보고하지 않는다.
        if factory.agent.get_language_backend().is_lsp():
            factory.agent.execute_task(factory.agent.get_language_server_manager_or_raise, name="verify_language_server_ready")
        # 프로젝트가 고정되어 있으므로 도구 목록은 서버 생성 시 한 번 등록한다.
        factory._set_mcp_tools(server, openai_tool_compatible=True)
        server.run(transport="streamable-http")
    finally:
        if factory.agent is not None:
            factory.agent.on_shutdown()


if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()
