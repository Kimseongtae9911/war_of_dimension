#pragma once

namespace wod_server {
	// 채널 하나를 Zone으로 구성
	class Zone 
	{
	public:
		Zone() = default;
		~Zone() = default;

		void Initialize();
		void Release();

		ZONE_STATE Update();

		void AddJob(IJob& job) { m_jobQueue.PushJob(job); }

	private:
		std::vector<CClient*> m_clients; // Zone에 속한 클라이언트들
		JobQueue m_jobQueue; // Zone에서 처리할 작업 큐
	};

}