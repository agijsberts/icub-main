// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#include <IRpcServer.h>
#include <yarp/os/idl/WireTypes.h>



class IRpcServer_get_answer : public yarp::os::Portable {
public:
  int32_t _return;
  virtual bool write(yarp::os::ConnectionWriter& connection) {
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(2)) return false;
    if (!writer.writeTag("get_answer",1,2)) return false;
    return true;
  }
  virtual bool read(yarp::os::ConnectionReader& connection) {
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListReturn()) return false;
    if (!reader.readI32(_return)) {
      reader.fail();
      return false;
    }
    return true;
  }
};

class IRpcServer_set_answer : public yarp::os::Portable {
public:
  int32_t rightAnswer;
  bool _return;
  virtual bool write(yarp::os::ConnectionWriter& connection) {
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(3)) return false;
    if (!writer.writeTag("set_answer",1,2)) return false;
    if (!writer.writeI32(rightAnswer)) return false;
    return true;
  }
  virtual bool read(yarp::os::ConnectionReader& connection) {
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListReturn()) return false;
    if (!reader.readBool(_return)) {
      reader.fail();
      return false;
    }
    return true;
  }
};

class IRpcServer_add_one : public yarp::os::Portable {
public:
  int32_t x;
  int32_t _return;
  virtual bool write(yarp::os::ConnectionWriter& connection) {
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(3)) return false;
    if (!writer.writeTag("add_one",1,2)) return false;
    if (!writer.writeI32(x)) return false;
    return true;
  }
  virtual bool read(yarp::os::ConnectionReader& connection) {
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListReturn()) return false;
    if (!reader.readI32(_return)) {
      reader.fail();
      return false;
    }
    return true;
  }
};

class IRpcServer_start : public yarp::os::Portable {
public:
  bool _return;
  virtual bool write(yarp::os::ConnectionWriter& connection) {
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(1)) return false;
    if (!writer.writeTag("start",1,1)) return false;
    return true;
  }
  virtual bool read(yarp::os::ConnectionReader& connection) {
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListReturn()) return false;
    if (!reader.readBool(_return)) {
      reader.fail();
      return false;
    }
    return true;
  }
};

class IRpcServer_stop : public yarp::os::Portable {
public:
  bool _return;
  virtual bool write(yarp::os::ConnectionWriter& connection) {
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(1)) return false;
    if (!writer.writeTag("stop",1,1)) return false;
    return true;
  }
  virtual bool read(yarp::os::ConnectionReader& connection) {
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListReturn()) return false;
    if (!reader.readBool(_return)) {
      reader.fail();
      return false;
    }
    return true;
  }
};

class IRpcServer_is_running : public yarp::os::Portable {
public:
  bool _return;
  virtual bool write(yarp::os::ConnectionWriter& connection) {
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(2)) return false;
    if (!writer.writeTag("is_running",1,2)) return false;
    return true;
  }
  virtual bool read(yarp::os::ConnectionReader& connection) {
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListReturn()) return false;
    if (!reader.readBool(_return)) {
      reader.fail();
      return false;
    }
    return true;
  }
};

int32_t IRpcServer::get_answer() {
  int32_t _return = 0;
  IRpcServer_get_answer helper;
  if (!yarp().canWrite()) {
    fprintf(stderr,"Missing server method '%s'?\n","int32_t IRpcServer::get_answer()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool IRpcServer::set_answer(const int32_t rightAnswer) {
  bool _return = false;
  IRpcServer_set_answer helper;
  helper.rightAnswer = rightAnswer;
  if (!yarp().canWrite()) {
    fprintf(stderr,"Missing server method '%s'?\n","bool IRpcServer::set_answer(const int32_t rightAnswer)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
int32_t IRpcServer::add_one(const int32_t x) {
  int32_t _return = 0;
  IRpcServer_add_one helper;
  helper.x = x;
  if (!yarp().canWrite()) {
    fprintf(stderr,"Missing server method '%s'?\n","int32_t IRpcServer::add_one(const int32_t x)");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool IRpcServer::start() {
  bool _return = false;
  IRpcServer_start helper;
  if (!yarp().canWrite()) {
    fprintf(stderr,"Missing server method '%s'?\n","bool IRpcServer::start()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool IRpcServer::stop() {
  bool _return = false;
  IRpcServer_stop helper;
  if (!yarp().canWrite()) {
    fprintf(stderr,"Missing server method '%s'?\n","bool IRpcServer::stop()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}
bool IRpcServer::is_running() {
  bool _return = false;
  IRpcServer_is_running helper;
  if (!yarp().canWrite()) {
    fprintf(stderr,"Missing server method '%s'?\n","bool IRpcServer::is_running()");
  }
  bool ok = yarp().write(helper,helper);
  return ok?helper._return:_return;
}

bool IRpcServer::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  reader.expectAccept();
  if (!reader.readListHeader()) { reader.fail(); return false; }
  yarp::os::ConstString tag = reader.readTag();
  while (!reader.isError()) {
    // TODO: use quick lookup, this is just a test
    if (tag == "get_answer") {
      int32_t _return;
      _return = get_answer();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeI32(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "set_answer") {
      int32_t rightAnswer;
      if (!reader.readI32(rightAnswer)) {
        reader.fail();
        return false;
      }
      bool _return;
      _return = set_answer(rightAnswer);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "add_one") {
      int32_t x;
      if (!reader.readI32(x)) {
        reader.fail();
        return false;
      }
      int32_t _return;
      _return = add_one(x);
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeI32(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "start") {
      bool _return;
      _return = start();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "stop") {
      bool _return;
      _return = stop();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (tag == "is_running") {
      bool _return;
      _return = is_running();
      yarp::os::idl::WireWriter writer(reader);
      if (!writer.isNull()) {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeBool(_return)) return false;
      }
      reader.accept();
      return true;
    }
    if (reader.noMore()) { reader.fail(); return false; }
    yarp::os::ConstString next_tag = reader.readTag();
    if (next_tag=="") break;
    tag = tag + "_" + next_tag;
  }
  return false;
}

