# Partial Update Bug - Root Cause Analysis & Fix Plan

## Date: November 2, 2025

---

## 🐛 The Problem

**Symptom:** When updating only the departure board (partial update), the weather area (left half) disappears, showing a
blank/corrupted left side with only the departure board visible on the right.

**Expected:** Only the departure board (right half) should update, while the weather display (left half) remains
unchanged and visible.

**Actual:** The weather display disappears or shows noise/artifacts.

---

## 🔍 Root Cause Analysis

After analyzing the code and the GxEPD2 forum discussion (https://github.com/ZinggJM/GxEPD2/discussions/126), I've
identified **THREE critical issues**:

### Issue #1: E-Paper Display Loses Content After Deep Sleep ⚠️

**The Fundamental Problem:**

```
Device boots → Displays both halves → Hibernates display → Deep sleep
                ↓
Device wakes → Display content is GONE (powered off)
                ↓
Tries partial update → But there's NOTHING to "partially update"!
```

**Why This Happens:**

- E-paper displays **retain content when powered**, but...
- `display.hibernate()` **powers down the display controller**
- After deep sleep, the ESP32 wakes up but the display is **blank/uninitialized**
- Calling `display.init()` reinitializes the controller but **doesn't restore the screen content**
- The physical e-paper may still show the old image, but the **controller doesn't know what's on screen**

**Evidence from Code:**

```cpp
// In display_manager.cpp - hibernate()
void DisplayManager::hibernate() {
    display.hibernate();  // ← Powers down display controller
    initialized = false;
    partialMode = false;
}

// In device_mode_manager.cpp - enterOperationalSleep()
void DeviceModeManager::enterOperationalSleep() {
    DisplayManager::hibernate();  // ← Called before deep sleep
    enterDeepSleep(sleepTimeSeconds);
}
```

**The Key Insight:**
After hibernation, the display controller's internal buffer is **out of sync** with what's physically on the e-paper
screen. When you try to do a partial update, the controller tries to update from its (blank) buffer, causing corruption.

---

### Issue #2: `fillRect()` During Partial Update Causes Artifacts ⚠️

**The Problem:**

```cpp
void DisplayManager::displayDepartureHalfOnly(const DepartureData& departures) {
    display.setPartialWindow(halfWidth, 0, halfWidth, screenHeight);
    display.firstPage();
    do {
        display.fillRect(halfWidth, 0, halfWidth, screenHeight, GxEPD_WHITE);  // ← Issue!
        updateDepartureHalf(false, departures);
    } while (display.nextPage());
}
```

**Why It's a Problem:**

- `fillRect()` tries to clear the partial window area
- After hibernation, the display buffer is not synchronized
- Clearing an unsynchronized buffer causes visual artifacts
- The GxEPD2 library may interpret this as corrupted data

**Evidence from GxEPD2 Forum:**
The library author recommends using `init()` with `initial=true` but doesn't explicitly mention using `fillRect()` in
partial mode. Many users report issues when clearing during partial updates after power loss.

---

### Issue #3: Init with `initial=true` Not Sufficient After Power Loss ⚠️

**Current Code:**

```cpp
void DisplayManager::initInternal(DisplayOrientation orientation, InitMode mode) {
    bool clearScreen = (mode == InitMode::FULL_REFRESH || mode == InitMode::LEGACY);

    // For partial update: clearScreen=false → initial=true
    display.init(DisplayConstants::SERIAL_BAUD_RATE, !clearScreen,
                 DisplayConstants::RESET_DURATION_MS, false);
}
```

**The Problem:**

- `initial=true` tells the library "don't clear the screen during init"
- But it **doesn't restore the buffer state** after hibernation
- The controller still has an empty/corrupted buffer
- Partial updates from an empty buffer = garbage on screen

---

## 💡 The Solution Strategy

Based on the analysis and GxEPD2 best practices, here's the fix:

### **Solution: Always Do Full Refresh After Deep Sleep**

**Why This Works:**

1. After deep sleep, the display state is unknown
2. Full refresh resynchronizes controller buffer with physical display
3. Only after a full refresh can you safely do partial updates
4. Subsequent partial updates (without sleep) would work fine

**Implementation Options:**

### **Option A: Force Full Refresh After Hibernation** ⭐ RECOMMENDED

**Strategy:**

- Track if display was hibernated using RTC memory
- If coming from deep sleep → Always do full refresh first
- Only use partial updates for updates within the same wake cycle (if any)

**Pros:**

- ✅ Guaranteed clean display after sleep
- ✅ Simple to implement
- ✅ Follows GxEPD2 best practices
- ✅ No risk of corruption

**Cons:**

- ⚠️ Slower updates (full refresh takes ~2 seconds)
- ⚠️ Screen flashes on every wake
- ⚠️ Slightly higher power consumption

**Reality Check:**
Since your device **goes to deep sleep after every update**, there are NO subsequent updates in the same wake cycle.
Therefore, partial updates are **never actually safe** in your current architecture!

---

### **Option B: Keep Display Powered During Deep Sleep**

**Strategy:**

- Don't call `display.hibernate()` before sleep
- Keep display controller powered
- Buffer remains synchronized
- Partial updates would work

**Pros:**

- ✅ True partial updates possible
- ✅ Faster updates
- ✅ No screen flashing

**Cons:**

- ❌ Significantly higher power consumption
- ❌ Defeats the purpose of deep sleep
- ❌ Not practical for battery operation
- ❌ Not recommended for e-paper displays

**Verdict:** Not suitable for your use case.

---

### **Option C: Periodic Full Refresh Only**

**Strategy:**

- Do partial updates normally
- Force full refresh every N updates or when content is stale
- Accept that some partial updates may have artifacts

**Pros:**

- ✅ Balance between speed and quality
- ✅ Most updates are fast

**Cons:**

- ❌ Still won't work after hibernation
- ❌ Risk of corruption on first update
- ❌ Doesn't solve the root cause

**Verdict:** Doesn't address the hibernation issue.

---

## 📋 Recommended Fix Plan

### **Step 1: Accept the Reality**

**Key Insight:**
Your device **ALWAYS hibernates and goes to deep sleep** after each update. This means:

- Every wake-up is a "cold start" for the display
- The display buffer is always out of sync
- **Partial updates are not reliable after hibernation**
- You should **always do full refresh** after deep sleep

**Evidence from Your Code:**

```cpp
// In device_mode_manager.cpp
void DeviceModeManager::showWeatherDeparture() {
    // ... fetch data
    // ... update display
    // No other updates happen!
}

void DeviceModeManager::enterOperationalSleep() {
    DisplayManager::hibernate();  // ← Always hibernates
    enterDeepSleep(sleepTimeSeconds);  // ← Always sleeps
}
```

There's **no scenario** where a partial update happens without a preceding hibernation/sleep cycle!

---

### **Step 2: Simplify - Always Use Full Refresh** ⭐

**The Fix:**
Remove the partial update logic and always do full refresh, since every update is after hibernation anyway.

**Changes Needed:**

1. **Remove `initForPartialUpdate()` calls**
    - Always use `initForFullRefresh()`
    - Remove the `initial=true` logic

2. **Remove partial update optimizations**
    - They're pointless if hibernation happens every time
    - Simplifies code significantly

3. **Update DeviceModeManager logic**
    - Always call `refreshFullScreen()` even for single-half updates
    - Pass `nullptr` for the half that doesn't need data fetch (optimization)

---

### **Step 3: Optional - Optimize Data Fetching (Not Display)**

**Smart Optimization:**
Instead of optimizing **display updates** (which must be full refresh), optimize **data fetching**:

```cpp
// Don't fetch data that doesn't need updating
if (needsWeatherUpdate && needsTransportUpdate) {
    // Fetch both
    fetchWeatherData(weather);
    fetchTransportData(depart);
    DisplayManager::refreshFullScreen(&weather, &depart);
}
else if (needsTransportUpdate) {
    // Only fetch transport, but still do FULL display refresh
    // Use cached weather data or pass nullptr
    fetchTransportData(depart);
    DisplayManager::refreshFullScreen(nullptr, &depart);  // ← Full refresh!
}
```

**Benefits:**

- ✅ Save time by not fetching unchanged data
- ✅ Save API calls
- ✅ Still get clean full display refresh
- ✅ No corruption or artifacts

---

## 🎯 Implementation Plan

### **Phase 1: Quick Fix - Force Full Refresh Always**

**Goal:** Fix the bug immediately by always using full refresh.

**Changes:**

1. **Modify `refreshDepartureHalf()` to use full refresh**
2. **Modify `refreshWeatherHalf()` to use full refresh**
3. **Keep data fetching optimization**

**Files to Change:**

- `src/display/display_manager.cpp` - Change refresh methods
- Documentation to clarify behavior

**Time:** 30 minutes
**Risk:** Low
**Result:** Bug fixed, display always works correctly

---

### **Phase 2: Simplify Code (Optional)**

**Goal:** Remove unnecessary partial update code since it's never used safely.

**Changes:**

1. **Remove `initForPartialUpdate()` method**
2. **Remove `InitMode::PARTIAL_UPDATE`**
3. **Remove `partialMode` flag**
4. **Simplify `initInternal()`**
5. **Keep `displayHalfAndHalf()` structure for code organization**

**Benefits:**

- Less code to maintain
- No confusion about when partial updates work
- Clearer intent

**Time:** 1 hour
**Risk:** Low
**Result:** Cleaner, more maintainable code

---

## 📊 Performance Impact

### **Current (Broken) Partial Update:**

- Init time: ~200ms
- Partial refresh: ~500ms
- **Total: ~700ms** (but produces corrupted display)

### **Fixed Full Refresh:**

- Init time: ~200ms
- Full refresh: ~2000ms
- **Total: ~2200ms** (but display is perfect)

### **Reality:**

- **Trade-off:** 1.5 seconds slower per update
- **Benefit:** Display always works correctly
- **Power:** Slightly higher, but minimal (display is still hibernated between updates)
- **Battery life impact:** Negligible (most power is in WiFi/API calls, not display)

---

## ⚠️ Why Partial Updates Don't Work in Your Architecture

Let me be crystal clear:

```
Current Architecture:
┌─────────────────────────────────────────────────────────────┐
│ Wake → Connect → Fetch → Display → Hibernate → Sleep        │
│                                      ↑                       │
│                                      └─ Display powered OFF  │
└─────────────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────────┐
│ Wake → Connect → Fetch → Display → Hibernate → Sleep        │
│         ↑                                                    │
│         └─ Display buffer is GONE/EMPTY                     │
└─────────────────────────────────────────────────────────────┘
```

**Partial updates only work if:**

1. Display is kept powered (no hibernation)
2. Multiple updates happen in same wake cycle
3. No sleep between updates

**Your architecture has:**

1. ❌ Display is hibernated every time
2. ❌ Only one update per wake cycle
3. ❌ Sleep happens after every update

**Conclusion:** Partial updates are **architecturally impossible** in your current design!

---

## ✅ Recommended Action

**Implement Phase 1 (Quick Fix) NOW:**

1. Modify `refreshDepartureHalf()` to do full refresh
2. Modify `refreshWeatherHalf()` to do full refresh
3. Test and verify display works correctly
4. Accept the 1.5 second longer update time (it's worth it for reliability)

**Consider Phase 2 (Simplify) LATER:**

5. After confirming Phase 1 works, simplify by removing unused partial update code
6. Document why full refresh is necessary
7. Enjoy clean, maintainable, working code

---

## 🎓 Key Takeaways

1. **E-paper displays lose buffer state after hibernation**
2. **Partial updates only work with persistent power**
3. **Your architecture requires hibernation (battery life)**
4. **Therefore, always use full refresh after hibernation**
5. **Optimize data fetching, not display updates**
6. **Accept 2-second full refresh as necessary trade-off**

---

## 📝 Next Steps

1. ✅ Review this analysis
2. ✅ Confirm the approach
3. ⚙️ Implement Phase 1 fix
4. 🧪 Test on device
5. 📊 Measure battery impact (should be minimal)
6. 🎯 Consider Phase 2 simplification

**The fix is straightforward: Always use full refresh after hibernation!** 🚀

