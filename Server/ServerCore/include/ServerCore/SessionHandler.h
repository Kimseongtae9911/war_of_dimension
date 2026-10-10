#pragma once
#include <ServerCore/Session.h>

namespace wod::core
{
// 서버별 콘텐츠 객체는 패킷 검증·전달과 disconnect 후속 처리를 확장한다.
template <class Session> class SessionHandler
{
  public:
    using SessionRef = std::shared_ptr<Session>;
    using Frame = FrameDecoder::Frame;
    virtual ~SessionHandler() = default;

    void Receive(size_t _bytes, typename Session::ContextType *_over)
    {
        auto session = GetTransportSession();
        const auto generation = session->Generation();
        std::vector<Frame> frames;
        if (!session->Decode(_bytes, *_over, frames))
        {
            Disconnect();
            return;
        }

        const auto count = frames.size();
        for (auto& frame : frames)
        {
            if (!ValidateFrame(frame) || !DispatchFrame(std::move(frame), session, generation))
            {
                Disconnect();
                return;
            }
        }
        session->Recv();
        OnReceiveComplete(count);
    }

    void Disconnect()
    {
        GetTransportSession()->Disconnect();
        OnDisconnectRequested();
    }

  protected:
    virtual SessionRef GetTransportSession() const = 0;
    virtual bool ValidateFrame(std::span<const char> _frame) const = 0;
    virtual bool DispatchFrame(Frame _frame, const SessionRef& _session, uint64_t _generation) = 0;

    virtual void OnReceiveComplete(size_t)
    {
    }

    virtual void OnDisconnectRequested()
    {
    }
};
}
