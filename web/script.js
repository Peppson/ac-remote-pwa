const tempDisplay = document.getElementById("tempDisplay");
const statusEl = document.getElementById("status");
const powerBtn = document.getElementById("powerBtn");
const modeGrid = document.getElementById("modeGrid");
const secondaryModeGrid = document.getElementById("secondaryModeGrid");
const fanBtn = document.getElementById("fanBtn");
const fanBtnDefaultContent = fanBtn ? fanBtn.innerHTML : "";
const sleepBtn = document.getElementById("sleepBtn");
const sleepBtnDefaultContent = sleepBtn ? sleepBtn.innerHTML : "";
const tempRingWrap = document.querySelector(".temp-ring-wrap");
const tempModeText = document.querySelector(".temp-mode-text");
const tempUnit = document.querySelector(".temp-center .temp-unit:not(.hidden)");
const tempRingFill = document.getElementById("tempRingFill");
const tempRingHead = document.getElementById("tempRingHead");
const tempArcGradStart = document.querySelector("#tempArcGradient .grad-start");
const tempArcGradEnd = document.querySelector("#tempArcGradient .grad-end");

const TEMP_RING_ARC = 445;
const TEMP_RING_CIRC = 534;
const TEMP_MIN = 18;
const TEMP_MAX = 32;
const TEMP_ARC_DEG = 300;
const RING_ROTATION_DEG = 120;
const RESPONSE_TIMEOUT_MS = 2500;
const STATUS_DURATION_MS = 4000;
const WS_RECONNECT_MS = 2000;

const FAN_VALUES = ["Silent", "Low", "Medium", "High"];
const MODE_THEME = {
    Cool: ["#0983ED", "#007aff"],
    Dry: ["#ffcf5a", "#f28c18"],
    Fan: ["#33AE95", "#2C9B86"],
    Auto: ["#6f6cff", "#4f56c7"]
};
const LOCK_DASH_HTML = `
  <svg
    class="btn-icon"
    viewBox="0 0 24 24"
    fill="none"
    stroke="currentColor"
    stroke-width="2"
    stroke-linecap="round"
    stroke-linejoin="round"
  >
    <path d="M5 12h14" />
  </svg>
`;

let responseTimerId = null;
let statusHideTimerId = null;
let wsConnection = null;
let wsReconnectTimerId = null;
let isDraggingTemp = false;

const acState = {
    power: 0,
    lid: 0,
    turbo: 0,
    sleep: 0,
    temp: 21,
    timer: 0,
    mode: "Cool",
    fan: "High"
};

document.addEventListener("touchstart", () => { }, { passive: true });


/*################  status  ################*/

function clearStatus() {
    if (statusHideTimerId !== null) {
        clearTimeout(statusHideTimerId);
        statusHideTimerId = null;
    }
    statusEl.className = "status";
    statusEl.style.visibility = "hidden";
    statusEl.textContent = "";
}

function clearResponseTimeout() {
    if (responseTimerId !== null) {
        clearTimeout(responseTimerId);
        responseTimerId = null;
    }
}

function showError(message = "Failed to send command") {
    if (statusHideTimerId !== null) {
        clearTimeout(statusHideTimerId);
    }
    statusEl.style.visibility = "visible";
    statusEl.textContent = message;
    statusHideTimerId = setTimeout(clearStatus, STATUS_DURATION_MS);
}


/*################  fan  ################*/

function getFanNumberFromValue(value) {
    const idx = FAN_VALUES.indexOf(value);
    return idx === -1 ? 1 : idx + 1;
}

function getFanValueFromNumber(number) {
    const idx = Math.max(1, Math.min(4, number)) - 1;
    return FAN_VALUES[idx];
}

function cycleFanValue() {
    const currentNumber = getFanNumberFromValue(acState.fan);
    const nextNumber = currentNumber >= 4 ? 1 : currentNumber + 1;
    setFanValue(getFanValueFromNumber(nextNumber));
}

function setFanValue(value) {
    acState.fan = value;

    if (fanBtn) {
        if (fanBtn.dataset.locked === "1") return;

        const level = getFanNumberFromValue(acState.fan);
        fanBtn.style.setProperty("--fan-level", String(level - 1));
        fanBtn.style.color = level === 1 ? "var(--muted)" : "var(--text)";
        fanBtn.style.setProperty("--fan-bg", "var(--btn-bg)");
        const fanItem = fanBtn.closest(".btn-container");
        const label = fanItem ? fanItem.querySelector(".btn-label") : null;

        if (label) {
            label.classList.toggle("active", level > 1);
        }
    }
}

function setFanLockedVisual(powered, fanEnabled) {
    if (!fanBtn) return;


    const fanItem = fanBtn.closest(".btn-container");
    const label = fanItem ? fanItem.querySelector(".btn-label") : null;

    fanBtn.disabled = !fanEnabled;
    fanBtn.style.pointerEvents = !fanEnabled ? "none" : "auto";

    if (powered && !fanEnabled) {
        fanBtn.dataset.locked = "1";
        fanBtn.classList.remove("active", "is-pressed");
        fanBtn.style.setProperty("--fan-bg", "var(--line)");
        fanBtn.style.color = "var(--muted)";
        fanBtn.innerHTML = LOCK_DASH_HTML;

        if (label) {
            label.classList.remove("active");
        }
        return;
    }

    if (powered && fanBtn.dataset.locked === "1") {
        fanBtn.dataset.locked = "0";
        fanBtn.innerHTML = fanBtnDefaultContent;
    }

    setFanValue(acState.fan);
}

function setSleepLockedVisual(powered, sleepEnabled) {
    if (!sleepBtn) return;

    const sleepItem = sleepBtn.closest(".btn-container");
    const label = sleepItem ? sleepItem.querySelector(".btn-label") : null;

    if (powered && !sleepEnabled) {
        sleepBtn.dataset.locked = "1";
        sleepBtn.classList.remove("active", "is-pressed");
        sleepBtn.style.color = "var(--muted)";
        sleepBtn.innerHTML = LOCK_DASH_HTML;

        if (label) {
            label.classList.remove("active");
        }
        return;
    }

    if (powered && sleepBtn.dataset.locked === "1") {
        sleepBtn.dataset.locked = "0";
        sleepBtn.innerHTML = sleepBtnDefaultContent;
    }

    sleepBtn.style.color = "";

    setSecondaryModeVisual("Sleep", acState.sleep === 1);
}

function isFanSpeedAllowed(mode) {
    return mode !== "Dry";
}

function isSleepAllowed(mode) {
    return mode !== "Fan" && mode !== "Auto";
}

function setSecondaryControlEnabled(mode, enabled) {
    if (!secondaryModeGrid) return;

    const item = secondaryModeGrid.querySelector('.btn-container[data-mode="' + mode + '"]');
    if (!item) return;

    const button = item.querySelector(".btn");
    if (!button) return;

    button.disabled = !enabled;
    button.style.pointerEvents = enabled ? "auto" : "none";
}

function applyModeSecondaryRules() {
    const powered = acState.power === 1;
    const sleepEnabled = powered && isSleepAllowed(acState.mode);
    const fanEnabled = powered && isFanSpeedAllowed(acState.mode);

    setFanLockedVisual(powered, fanEnabled);
    setSleepLockedVisual(powered, sleepEnabled);

    setSecondaryControlEnabled("Sleep", sleepEnabled);
    setSecondaryControlEnabled("Lid", powered);
    setSecondaryControlEnabled("Turbo", powered);
}


/*################  temp  ################*/

function updateTempReadout() {
    if (!tempDisplay) return;

    if (tempUnit) {
        tempUnit.classList.toggle("hidden", acState.mode !== "Cool");
    }

    if (acState.power === 0) {
        return;
    }

    if (acState.mode === "Cool") {
        tempDisplay.classList.remove("mode-icon-readout");
        tempDisplay.textContent = String(acState.temp);
        return;
    }

    const existingIcon = tempDisplay.querySelector(".temp-display-icon");
    if (tempDisplay.classList.contains("mode-icon-readout") &&
        existingIcon &&
        existingIcon.dataset.mode === acState.mode
    ) {
        return;
    }

    const sourceIcon = modeGrid
        ? modeGrid.querySelector('.btn-container[data-mode="' + acState.mode + '"] .btn-icon')
        : null;

    if (!sourceIcon) return;

    const readoutIcon = sourceIcon.cloneNode(true);
    readoutIcon.classList.add("temp-display-icon");
    readoutIcon.dataset.mode = acState.mode;

    if (acState.mode === "Dry") {
        readoutIcon.classList.add("shiver-heat");
    } else if (acState.mode === "Fan") {
        readoutIcon.classList.add("spin-fast");
    } else if (acState.mode === "Auto") {
        readoutIcon.classList.add("spin-slow");
    }

    tempDisplay.classList.add("mode-icon-readout");
    tempDisplay.textContent = "";
    tempDisplay.appendChild(readoutIcon);
}

function updateTempArcVisual() {
    if (!tempRingFill || !tempRingHead) return;

    if (acState.mode === "Cool") {
        if (tempArcGradStart) tempArcGradStart.setAttribute("stop-color", "#007aff");
        if (tempArcGradEnd) tempArcGradEnd.setAttribute("stop-color", "#33AE95");

        const t = (acState.temp - TEMP_MIN) / (TEMP_MAX - TEMP_MIN);
        const fill = (t * TEMP_RING_ARC).toFixed(1);
        tempRingFill.style.strokeDasharray = fill + " " + TEMP_RING_CIRC;
        tempRingHead.style.strokeDashoffset = "-" + fill;
        tempRingHead.style.opacity = "1";
        return;
    }

    const theme = MODE_THEME[acState.mode] || MODE_THEME.Cool;
    if (tempArcGradStart) tempArcGradStart.setAttribute("stop-color", theme[0]);
    if (tempArcGradEnd) tempArcGradEnd.setAttribute("stop-color", theme[1]);

    tempRingFill.style.strokeDasharray = TEMP_RING_ARC + " " + TEMP_RING_CIRC;
    tempRingHead.style.strokeDashoffset = "-" + TEMP_RING_ARC;
    tempRingHead.style.opacity = "0";
}

function setTempValue(value) {
    const next = Math.max(TEMP_MIN, Math.min(TEMP_MAX, value));
    acState.temp = next;
    updateTempReadout();
    updateTempArcVisual();
}

function getTempFromPointerEvent(event) {
    if (!tempRingWrap) return null;
    const rect = tempRingWrap.getBoundingClientRect();
    const centerX = rect.left + rect.width / 2;
    const centerY = rect.top + rect.height / 2;
    const dx = event.clientX - centerX;
    const dy = event.clientY - centerY;

    let angle = Math.atan2(dy, dx) * 180 / Math.PI;
    if (angle < 0) angle += 360;

    let unrotated = angle - RING_ROTATION_DEG;
    if (unrotated < 0) unrotated += 360;

    let t;
    if (unrotated <= TEMP_ARC_DEG) {
        t = unrotated / TEMP_ARC_DEG;
    } else {
        // Pointer in the 60deg gap snaps to nearest end.
        t = unrotated < 330 ? 1 : 0;
    }

    return Math.round(TEMP_MIN + t * (TEMP_MAX - TEMP_MIN));
}

function updateTempFromPointer(event) {
    const next = getTempFromPointerEvent(event);
    if (next === null || Number.isNaN(next)) return;
    if (next === acState.temp) return;
    setTempValue(next);
    if (isDraggingTemp) {
        sendNow();
    }
}

function updateTempInteractivity() {
    const tempEnabled = acState.power === 1 && acState.mode === "Cool";

    if (tempRingWrap) {
        tempRingWrap.style.pointerEvents = tempEnabled ? "auto" : "none";
    }
}

function setupTempRingInput() {
    if (!tempRingWrap) return;

    tempRingWrap.addEventListener("pointerdown", (event) => {
        event.preventDefault();
        clearTurboIfActive();
        isDraggingTemp = true;
        tempRingWrap.setPointerCapture(event.pointerId);
        updateTempFromPointer(event);
    });

    tempRingWrap.addEventListener("pointermove", (event) => {
        if (!isDraggingTemp) return;
        updateTempFromPointer(event);
    });

    tempRingWrap.addEventListener("pointerup", (event) => {
        if (!isDraggingTemp) return;
        updateTempFromPointer(event);
        isDraggingTemp = false;
        tempRingWrap.releasePointerCapture(event.pointerId);
    });

    tempRingWrap.addEventListener("pointercancel", (event) => {
        if (!isDraggingTemp) return;
        isDraggingTemp = false;
        tempRingWrap.releasePointerCapture(event.pointerId);
    });
}


/*################  websocket  ################*/

function connectStateSocket() {
    if (wsConnection && (wsConnection.readyState === WebSocket.OPEN || wsConnection.readyState === WebSocket.CONNECTING)) {
        return;
    }

    const protocol = location.protocol === "https:" ? "wss" : "ws";
    const url = protocol + "://" + location.hostname + ":81/";
    wsConnection = new WebSocket(url);

    wsConnection.onopen = () => {
        if (wsReconnectTimerId !== null) {
            clearTimeout(wsReconnectTimerId);
            wsReconnectTimerId = null;
        }
    };

    wsConnection.onmessage = (event) => {
        try {
            const state = JSON.parse(event.data);
            clearResponseTimeout();

            if (state && typeof state.error === "string" && state.error.length > 0) {
                showError(state.error);
                return;
            }

            applyServerState(state);
            clearStatus();
        } catch (_) {
            // Ignore malformed payloads.
        }
    };

    wsConnection.onclose = scheduleStateSocketReconnect;
    wsConnection.onerror = scheduleStateSocketReconnect;
}

function scheduleStateSocketReconnect() {
    if (wsReconnectTimerId !== null) return;

    wsReconnectTimerId = setTimeout(() => {
        wsReconnectTimerId = null;
        connectStateSocket();
    }, WS_RECONNECT_MS);
}

function sendNow() {
    clearStatus();
    clearResponseTimeout();

    const payload = {
        power: acState.power,
        lid: acState.lid,
        turbo: acState.turbo,
        sleep: acState.sleep,
        temp: acState.temp,
        timer: acState.timer,
        mode: acState.mode,
        fan: acState.fan
    };

    try {
        if (!wsConnection || wsConnection.readyState !== WebSocket.OPEN) {
            throw new Error("Socket not connected");
        }

        wsConnection.send(JSON.stringify(payload));
        responseTimerId = setTimeout(() => {
            responseTimerId = null;
            showError("No response from server");
        }, RESPONSE_TIMEOUT_MS);

    } catch (e) {
        showError();
        clearResponseTimeout();
    }
}

function applyServerState(state) {
    if (!state || typeof state !== "object") return;

    const turbo = state.turbo === 1 || state.turbo === "1" || state.turbo === true;
    const sleep = state.sleep === 1 || state.sleep === "1" || state.sleep === true;
    const mode = typeof state.mode === "string" ? state.mode : acState.mode;
    const fan = typeof state.fan === "string" ? state.fan : acState.fan;
    const tempValue = Number(state.temp);
    const timerValue = Number(state.timer);

    acState.power = state.power === 1 || state.power === "1" || state.power === true ? 1 : 0;
    acState.lid = state.lid === 1 || state.lid === "1" || state.lid === true ? 1 : 0;

    setPowerState(acState.power === 1);
    setLidState(acState.lid === 1);
    setJetState(turbo);
    setSleepState(turbo ? false : sleep);

    if (mode === "Cool" || mode === "Dry" || mode === "Fan" || mode === "Auto") {
        acState.mode = mode;
        setActiveModeVisual(acState.mode);
    }

    if (fan === "High" || fan === "Medium" || fan === "Low" || fan === "Silent") {
        setFanValue(fan);
    }

    if (!Number.isNaN(tempValue)) {
        setTempValue(tempValue);
    }

    if (!Number.isNaN(timerValue)) {
        acState.timer = Math.max(0, Math.min(10, timerValue));
    }

    applyModeSecondaryRules();
}


/*################  states  ################*/

function applyModeTheme(modeValue) {
    const theme = MODE_THEME[modeValue] || MODE_THEME.Cool;
    document.documentElement.style.setProperty("--mode-accent-start", theme[0]);
    document.documentElement.style.setProperty("--mode-accent-end", theme[1]);
}

function updateModeText(modeValue) {
    if (!tempModeText) return;

    tempModeText.textContent = "Mode: " + modeValue;
}

function updateCoolSnowVisual() {
    document.body.classList.toggle("mode-cool-active", acState.power === 1 && acState.mode === "Cool");
}

function setActiveModeVisual(value) {
    if (!modeGrid) return;

    applyModeTheme(value);

    modeGrid.querySelectorAll(".btn-container[data-mode]").forEach((item) => {
        const active = item.getAttribute("data-mode") === value;
        item.querySelector(".btn").classList.toggle("active", active);
        item.querySelector(".btn-label").classList.toggle("active", active);
    });

    updateModeText(value);
    updateTempReadout();
    updateTempArcVisual();
    updateTempInteractivity();
    updateCoolSnowVisual();
}

function setSecondaryModeVisual(mode, active) {
    if (!secondaryModeGrid) return;

    const item = secondaryModeGrid.querySelector('.btn-container[data-mode="' + mode + '"]');
    if (!item) return;

    const button = item.querySelector(".btn");
    const label = item.querySelector(".btn-label");

    if (button) {
        button.classList.toggle("active", active);
    }

    if (label) {
        label.classList.toggle("active", active);
    }
}

function setLidState(next) {
    acState.lid = next ? 1 : 0;
    setSecondaryModeVisual("Lid", acState.lid === 1);
}

function setSleepState(next) {
    acState.sleep = next ? 1 : 0;
    setSecondaryModeVisual("Sleep", acState.sleep === 1);
}

function setJetState(next) {
    acState.turbo = next ? 1 : 0;
    setSecondaryModeVisual("Turbo", acState.turbo === 1);
}

function setControlsInteractive(enabled) {
    document.querySelectorAll("button").forEach((button) => {
        if (button === powerBtn) return;

        button.disabled = !enabled;
        button.style.pointerEvents = enabled ? "auto" : "none";
    });

    applyModeSecondaryRules();
    updateTempInteractivity();
}

function activateTurboPreset() {
    acState.mode = "Cool";
    setActiveModeVisual(acState.mode);
    setTempValue(TEMP_MIN);
    setFanValue("High");
    setSleepState(false);
    setJetState(true);
    applyModeSecondaryRules();
}

function clearTurboIfActive() {
    if (acState.turbo !== 1) return;

    setJetState(false);
}

function setPowerState(next) {
    acState.power = next ? 1 : 0;
    powerBtn.classList.toggle("active", acState.power === 1);

    document.body.classList.toggle("power-off", acState.power === 0);
    updateCoolSnowVisual();
    setControlsInteractive(acState.power === 1);
    updateModeText(acState.mode);
    updateTempReadout();
}


/*################  events  ################*/

document.querySelectorAll(".btn").forEach((button) => {
    const add = () => button.classList.add("is-pressed");
    const remove = () => button.classList.remove("is-pressed");

    button.addEventListener("pointerdown", add);
    button.addEventListener("pointerup", remove);
    button.addEventListener("pointercancel", remove);
    button.addEventListener("pointerleave", remove);
});

modeGrid.addEventListener("click", (e) => {
    if (acState.power === 0) return;

    const item = e.target.closest(".btn-container[data-mode]");
    if (!item) return;

    const selectedMode = item.getAttribute("data-mode");

    clearTurboIfActive();
    if (selectedMode !== acState.mode) {
        setSleepState(false);
    }

    acState.mode = selectedMode;
    setActiveModeVisual(acState.mode);
    applyModeSecondaryRules();
    sendNow();
});

secondaryModeGrid.addEventListener("click", (e) => {
    if (acState.power === 0) return;

    const item = e.target.closest(".btn-container[data-mode]");
    if (!item || item.contains(fanBtn)) return;

    const selectedMode = item.getAttribute("data-mode");
    const isActive = item.querySelector(".btn").classList.contains("active");

    if (selectedMode !== "Turbo") {
        clearTurboIfActive();
    }

    if (selectedMode === "Lid") {
        setLidState(isActive ? false : true);

    } else if (selectedMode === "Turbo") {
        isActive ? setJetState(false) : activateTurboPreset();

    } else if (selectedMode === "Sleep") {
        if (!isSleepAllowed(acState.mode)) return;
        setSleepState(isActive ? false : true);
    }

    sendNow();
});

if (fanBtn) {
    fanBtn.addEventListener("click", () => {
        if (acState.power === 0 || !isFanSpeedAllowed(acState.mode)) return;
        clearTurboIfActive();
        cycleFanValue();
        sendNow();
    });
}

powerBtn.addEventListener("click", () => {
    setPowerState(acState.power === 0);
    sendNow();
});


setActiveModeVisual(acState.mode);
setPowerState(acState.power === 1);
setLidState(acState.lid === 1);
setJetState(acState.turbo === 1);
setSleepState(acState.sleep === 1);
setFanValue(acState.fan);
setTempValue(acState.temp);
applyModeSecondaryRules();
setupTempRingInput();
connectStateSocket();


// dev
/* acState.power = 1;
setPowerState(acState.power === 1); */
