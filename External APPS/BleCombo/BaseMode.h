#pragma once

class BaseMode {
public:
  virtual ~BaseMode() {}
  virtual void enter() = 0;
  virtual void exit() = 0;
  virtual void loop() = 0;
};
