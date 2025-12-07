#include "openxr_api.h"
#include "platform/input_manager.h"
#include "utils/logger.h"
#include <mutex>
#include <queue>
#include <cstring>
#include <unordered_map>
#include <memory>

// External declarations
extern std::mutex g_instanceMutex;
extern std::unordered_map<XrInstance, std::shared_ptr<XRInstance>> g_instances;

struct XREvent {
    XrEventDataBuffer buffer;
    bool valid;
};

static std::mutex g_eventMutex;
static std::queue<XREvent> g_eventQueue;

XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
    if (!eventData) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Validate instance
    std::lock_guard<std::mutex> instLock(g_instanceMutex);
    auto instIt = g_instances.find(instance);
    if (instIt == g_instances.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Check event queue
    std::lock_guard<std::mutex> eventLock(g_eventMutex);
    
    if (g_eventQueue.empty()) {
        return XR_EVENT_UNAVAILABLE;
    }
    
    XREvent event = g_eventQueue.front();
    g_eventQueue.pop();
    
    if (event.valid && eventData->type == event.buffer.type) {
        memcpy(eventData, &event.buffer, sizeof(XrEventDataBuffer));
        return XR_SUCCESS;
    }
    
    return XR_EVENT_UNAVAILABLE;
}

// Helper function to post events
void PostEvent(const XrEventDataBuffer& event) {
    std::lock_guard<std::mutex> lock(g_eventMutex);
    
    XREvent xrEvent;
    xrEvent.buffer = event;
    xrEvent.valid = true;
    
    g_eventQueue.push(xrEvent);
}

// Post session state changed event
void PostSessionStateChangedEvent(XrSession session, XrSessionState state) {
    XrEventDataSessionStateChanged event = {};
    event.type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
    event.next = nullptr;
    event.session = session;
    event.state = state;
    event.time = GetCurrentXrTime();
    
    XrEventDataBuffer buffer = {};
    buffer.type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
    memcpy(&buffer, &event, sizeof(event));
    
    PostEvent(buffer);
}

// Post instance loss pending event
void PostInstanceLossPendingEvent(XrInstance instance) {
    XrEventDataInstanceLossPending event = {};
    event.type = XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING;
    event.next = nullptr;
    event.instance = instance;
    event.lossTime = GetCurrentXrTime();
    
    XrEventDataBuffer buffer = {};
    buffer.type = XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING;
    memcpy(&buffer, &event, sizeof(event));
    
    PostEvent(buffer);
}

// Post interaction profile changed event
void PostInteractionProfileChangedEvent(XrSession session) {
    XrEventDataInteractionProfileChanged event = {};
    event.type = XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED;
    event.next = nullptr;
    event.session = session;
    
    XrEventDataBuffer buffer = {};
    buffer.type = XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED;
    memcpy(&buffer, &event, sizeof(event));
    
    PostEvent(buffer);
}

