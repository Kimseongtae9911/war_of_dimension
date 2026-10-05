#pragma once
class CGameFramework;
struct ID3D12Device;
struct ID3D12Resource;
class ParticleBufferPool;
bool ClientMemoryProfileActive();
bool ClientMemoryProfileLegacyGeometry();
bool ClientMemoryProfileFullHeroParts();
bool ClientMemoryProfileFullParticlePool();
bool ClientMemoryProfileDedicatedParticleBuffers();
void ClientMemoryProfileRecordHeroModel(unsigned int skins, unsigned int animatedFrames, unsigned long long matrixBytes);
void ClientMemoryProfileSnapshot(ID3D12Device* device, const char* phase, const ParticleBufferPool* pool = nullptr);
void ClientMemoryProfileRecordParticle(ID3D12Device* device, ID3D12Resource* streamOutput,
    ID3D12Resource* draw, unsigned int capacity, unsigned int stride);
int RunClientMemoryProfile(CGameFramework& framework, const wchar_t* reportPath, bool legacy, bool heroMeasurement = false, bool fullHeroParts = false, bool particleMeasurement = false, bool fullParticlePool = false, bool measureReuse = false, bool dedicatedBuffers = false);
int RunParticleSelectionTests(const wchar_t* reportPath);
int RunHeroSelectionAudit(CGameFramework& framework, const wchar_t* reportPath);
