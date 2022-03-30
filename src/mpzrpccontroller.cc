#include "mpzrpccontroller.h"

MpzrpcController::MpzrpcController()
{
    m_failed = false;
    m_errText = "";
}

void MpzrpcController::Reset()
{
    m_failed = false;
    m_errText = "";
}

bool MpzrpcController::Failed() const
{
    return m_failed;
}

std::string MpzrpcController::ErrorText() const
{
    return m_errText;
}

void MpzrpcController::SetFailed(const std::string &reason)
{
    m_failed = true;
    m_errText = reason;
}

// 目前未实现具体的功能
void MpzrpcController::StartCancel() {}
bool MpzrpcController::IsCanceled() const { return false; }
void MpzrpcController::NotifyOnCancel(google::protobuf::Closure *callback) {}