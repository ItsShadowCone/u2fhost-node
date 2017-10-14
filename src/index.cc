#include <node.h>

#include <u2f-host/u2f-host.h>

#include <chrono>
#include <thread>

#define U2F_HOST_RESPONSE_SIZE 4096

namespace u2fhostnode {

using namespace node;
using namespace v8;

enum u2fh_action {
    U2FHostRegister = 1,
    U2FHostAuthenticate = 2,
};

static u2fh_devs *u2fh_devices;
static bool u2fh_ready = false;

void ErrorToString(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  args.GetReturnValue().Set(args.This()->Get(String::NewFromUtf8(isolate, "message")));
}

void ThrowException(Isolate* isolate, const char* message, int code) {
    Local<Object> obj = Object::New(isolate);
    obj->Set(String::NewFromUtf8(isolate, "message"), String::NewFromUtf8(isolate, message));
    obj->Set(String::NewFromUtf8(isolate, "code"), Integer::New(isolate, code));

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, ErrorToString);
    Local<Function> fn = tpl->GetFunction();
    fn->SetName(String::NewFromUtf8(isolate, "toString"));
    obj->Set(String::NewFromUtf8(isolate, "toString"), fn);

    isolate->ThrowException(obj);
}

bool DetectU2FDevices(int Counter) {
    unsigned int* MaxIndex = NULL;
    u2fh_rc rc = u2fh_devs_discover(u2fh_devices, MaxIndex);
    if(rc == U2FH_NO_U2F_DEVICE && Counter < 15) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return DetectU2FDevices(Counter + 1);
    } else if(rc != U2FH_OK) {
        return false;
    }
    return true;
}

int PerformU2FAction(u2fh_action Action, const char* challenge, const char* origin, char* response, size_t response_len) {
    // If U2F isn't ready and doesn't get ready within the next 5 seconds
/*    for (int i = 0; i < 10 && !u2fh_ready; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(0.5));
    }*/

    if(!u2fh_ready) {
        snprintf(response, response_len-1, "Could not execute U2F function: U2F not ready");
        response[response_len-1] = '\0';
        return 1;
    }

    bool DetectedDevices = DetectU2FDevices(0);
    if(!DetectedDevices) {
        snprintf(response, response_len-1, "No devices found. Timeout");
        response[response_len-1] = '\0';
        return 1;
    }

    u2fh_rc rc = U2FH_OK;

    if(Action == U2FHostRegister)
        rc = u2fh_register2(u2fh_devices, challenge, origin, response, &response_len, U2FH_REQUEST_USER_PRESENCE);

    if(Action == U2FHostAuthenticate)
        rc = u2fh_authenticate2(u2fh_devices, challenge, origin, response, &response_len, U2FH_REQUEST_USER_PRESENCE);

    if(rc != U2FH_OK) {
        snprintf(response, response_len-1, "Registering/Authenticating failed (%d): %s", (int)rc, u2fh_strerror(rc));
        response[response_len-1] = '\0';
        return (int)rc;
    }
    return 0;
}

void Register(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if(args.Length() != 2)
        return ThrowException(isolate, "Wrong number of arguments", 1);

    if(!args[0]->IsString() || !args[1]->IsString())
        return ThrowException(isolate, "Wrong argument types", 1);

    String::Utf8Value challenge(args[0]);
    String::Utf8Value origin(args[1]);

    if(!*challenge || !*origin)
        return ThrowException(isolate, "Could not parse strings", 1);

    char response[U2F_HOST_RESPONSE_SIZE];
    int ret = PerformU2FAction(u2fh_action::U2FHostRegister, *challenge, *origin, response, U2F_HOST_RESPONSE_SIZE);

    if(ret != 0)
        return ThrowException(isolate, response, ret);

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, response));
}

void Sign(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if(args.Length() != 2)
        return ThrowException(isolate, "Wrong number of arguments", 1);

    if(!args[0]->IsString() || !args[1]->IsString())
        return ThrowException(isolate, "Wrong argument types", 1);

    String::Utf8Value challenge(args[0]);
    String::Utf8Value origin(args[1]);

    if(!*challenge || !*origin)
        return ThrowException(isolate, "Could not parse strings", 1);

    char response[U2F_HOST_RESPONSE_SIZE];
    int ret = PerformU2FAction(u2fh_action::U2FHostAuthenticate, *challenge, *origin, response, U2F_HOST_RESPONSE_SIZE);

    if(ret != 0)
        return ThrowException(isolate, response, ret);

    args.GetReturnValue().Set(String::NewFromUtf8(isolate, response));
}

static void ShutdownU2F(void* context) {
    Isolate* isolate = static_cast<Isolate*>(context);
    HandleScope scope(isolate);

    u2fh_ready = false;
    u2fh_devs_done(u2fh_devices);
    u2fh_global_done();
}

void Initialize(Local<Object> exports) {
    Isolate* isolate = exports->GetIsolate();
    HandleScope scope(isolate);

    Local<Boolean> debug = isolate->GetCurrentContext()->Global()->Get(v8::String::NewFromUtf8(isolate, "U2F_DEBUG"))->ToBoolean();

    u2fh_rc rc = u2fh_global_init(debug->Value() ? U2FH_DEBUG : (u2fh_initflags)0);
    if (rc != U2FH_OK) {
        char err[U2F_HOST_RESPONSE_SIZE];

        snprintf(err, U2F_HOST_RESPONSE_SIZE-1, "Could not initialize U2F (%d): %s", (int)rc, u2fh_strerror(rc));
        err[U2F_HOST_RESPONSE_SIZE-1] = '\0';

        ThrowException(isolate, err, (int)rc);
        return;
    }

    rc = u2fh_devs_init(&u2fh_devices);
    if (rc != U2FH_OK) {
        char err[U2F_HOST_RESPONSE_SIZE];

        snprintf(err, U2F_HOST_RESPONSE_SIZE-1, "Could not initialize U2F devices (%d): %s", (int)rc, u2fh_strerror(rc));
        err[U2F_HOST_RESPONSE_SIZE-1] = '\0';

        ThrowException(isolate, err, (int)rc);
        return;
    }

    u2fh_ready = true;

    NODE_SET_METHOD(exports, "register", Register);
    NODE_SET_METHOD(exports, "sign", Sign);
    AtExit(ShutdownU2F, isolate);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

}
