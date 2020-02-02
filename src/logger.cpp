Logger TheLogger;
thread_local WCHAR Logger::Buffer[1024];

void Logger::RegisterOutput(IOutput* output) {
  auto lock = mutex_.lock_exclusive();
  outputs_.push_back(output);
}

void Logger::UnregisterOutput(IOutput* output) {
  auto lock = mutex_.lock_exclusive();
  outputs_.erase(std::remove(outputs_.begin(), outputs_.end(), output),
                 outputs_.end());
}

void Logger::Print(PCWSTR str) {
  auto lock = mutex_.lock_shared();
  for (auto sink : outputs_) sink->Print(str);
}

LoggerRegistration::LoggerRegistration() : output_{nullptr} {}

LoggerRegistration::LoggerRegistration(IOutput* output) : output_{output} {
  TheLogger.RegisterOutput(output);
}

LoggerRegistration::~LoggerRegistration() { Terminate(); }

LoggerRegistration::LoggerRegistration(LoggerRegistration&& other) {
  operator=(std::move(other));
}

LoggerRegistration& LoggerRegistration::operator=(LoggerRegistration&& other) {
  if (output_ != nullptr) TheLogger.UnregisterOutput(output_);
  output_ = other.output_;
  other.output_ = nullptr;
  return *this;
}

void LoggerRegistration::Terminate() {
  if (output_ != nullptr) {
    TheLogger.UnregisterOutput(output_);
    output_ = nullptr;
  }
}

LoggerRegistration RegisterLoggerOutput(IOutput* output) {
  _ASSERT(output != nullptr);
  return output != nullptr ? LoggerRegistration{output} : LoggerRegistration{};
}