#pragma once

namespace wod_server {
	template<class T>
	class TSingleton abstract
	{
	public:
		virtual bool Initialize() abstract;
		virtual bool Release() abstract;
		
		static T* GetInstance() {
			if (!m_instance)
				m_instance.reset(new T());
			return m_instance.get();
		}

		void DestroyInstance() {		
			if (m_instance)
				m_instance.reset(nullptr);
		}

		virtual ~TSingleton() = default;

	protected:
		TSingleton() {}
		TSingleton(T const&) = delete;
		TSingleton& operator=(const T&) = delete;

	protected:
		static std::unique_ptr<T> m_instance;
	};

	class ISkillHandler abstract
	{
	public:
		virtual void Handle() abstract;
	};

	struct SkillCsv;
	class ISkillObject abstract
	{
	public:
		volatile bool active = false;

		void SetObjectType(SKILL_TYPE type) { m_type = type; }

		void SetPower(int power) { m_power = power; }
		int GetPower() const { return m_power; }
		void SetCritical(int critical) { m_critical = critical; }
		void SetClientID(int id) { m_clientID = id; }
		int GetClientID() const { return m_clientID; }

	protected:
		SKILL_TYPE m_type;
		int m_power;
		int m_critical;
		int m_clientID;
		float m_damageReduction = 0.f;
		SkillCsv* m_skillCsv = nullptr;
	};

	class IItem abstract
	{
	public:
		virtual bool Use() = 0;
	};
}