import VerticalLiftBridgeController from './bridgeController.js';

document.addEventListener('DOMContentLoaded', () => {
  const statusEl = document.getElementById('ledStatus');
  const baseURL  = window.location.origin;

  const MODE_ENDPOINT    = `${baseURL}/mode`;
  const OPEN_ENDPOINT    = '/led/on';
  const CLOSE_ENDPOINT   = '/led/off';
  const DIST_ENDPOINT    = `${baseURL}/distance`;
  const STATE_ENDPOINT   = `${baseURL}/state`;
  const LIGHTS_ENDPOINT  = `${baseURL}/lights`;
  const TIMERS_ENDPOINT  = `${baseURL}/timers`;

  function sendRequest(endpoint, opts = {}) {
    return fetch(`${baseURL}${endpoint}`, opts)
      .then(res => res.text())
      .then(text => {
        if (statusEl) statusEl.textContent = `Status: ${text}`;
        return text;
      })
      .catch(err => {
        console.error(err);
        if (statusEl) statusEl.textContent = 'Status: Error';
      });
  }

  const modeSwitch = document.getElementById('modeSwitch');
  const modeLabel  = document.getElementById('modeLabel');
  const btnOpen    = document.getElementById('Open');
  const btnClose   = document.getElementById('Close');
  const boatDistA  = document.getElementById('boatDistA');
  const boatDistB  = document.getElementById('boatDistB');

  // Lamps
  const road = {
    red:    document.getElementById('road-red'),
    yellow: document.getElementById('road-yellow'),
    green:  document.getElementById('road-green'),
    timer:  document.getElementById('road-timer'),
  };
  const ship = {
    red:    document.getElementById('ship-red'),
    yellow: document.getElementById('ship-yellow'),
    green:  document.getElementById('ship-green'),
    timer:  document.getElementById('ship-timer'),
  };

  let isManual = false;

  // Bridge UI
  const bridge = new VerticalLiftBridgeController('bridgeRoot');

  // Init
  initMode().then(() => {
    if (btnOpen)  btnOpen.addEventListener('click', onOpen);
    if (btnClose) btnClose.addEventListener('click', onClose);
  });

  async function initMode() {
    try {
      const r = await fetch(MODE_ENDPOINT, { method: 'GET' });
      if (r.ok) {
        const data = await r.json().catch(() => null);
        isManual = data?.value === 'manual';
      }
    } catch {}

    if (modeSwitch) modeSwitch.checked = isManual;
    applyModeUI();
    await setDeviceMode(isManual ? 'manual' : 'auto');

    if (modeSwitch) modeSwitch.addEventListener('change', onModeToggle);

    startDistancePolling();
    startStatePolling();
    startLightsPolling();
    startTimerPolling();
  }

  function applyModeUI() {
    if (modeLabel) modeLabel.textContent = `Mode: ${isManual ? 'Manual' : 'Automatic'}`;
    if (btnOpen)  btnOpen.disabled  = !isManual;
    if (btnClose) btnClose.disabled = !isManual;
    bridge.setManualEnabled?.(isManual);
  }

  async function onModeToggle() {
    isManual = modeSwitch.checked;
    applyModeUI();
    await setDeviceMode(isManual ? 'manual' : 'auto');
  }

  function setDeviceMode(value) {
    return fetch(`${MODE_ENDPOINT}?value=${encodeURIComponent(value)}`, { method: 'POST' })
      .then(r => r.text())
      .then(t => console.log('Mode set:', t))
      .catch(e => console.error('Failed to set mode', e));
  }

  async function onOpen() {
    if (!isManual) return;
    await sendRequest(OPEN_ENDPOINT);
  }

  async function onClose() {
    if (!isManual) return;
    await sendRequest(CLOSE_ENDPOINT);
  }

  // ===== Distances A & B =====
  function startDistancePolling() {
    const poll = async () => {
      try {
        const r = await fetch(DIST_ENDPOINT, { cache: 'no-store' });
        if (!r.ok) throw new Error('bad response');
        const data = await r.json();
        const a = typeof data.A === 'number' ? data.A.toFixed(1) : '--';
        const b = typeof data.B === 'number' ? data.B.toFixed(1) : '--';
        if (boatDistA) boatDistA.textContent = `Sensor A: ${a} cm`;
        if (boatDistB) boatDistB.textContent = `Sensor B: ${b} cm`;
      } catch (e) {
        if (boatDistA) boatDistA.textContent = `Sensor A: --`;
        if (boatDistB) boatDistB.textContent = `Sensor B: --`;
      }
    };
    poll();
    setInterval(poll, 500);
  }

  // ===== Bridge state =====
  function startStatePolling() {
    const poll = async () => {
      try {
        const r = await fetch(STATE_ENDPOINT, { cache: 'no-store' });
        if (!r.ok) throw new Error('bad response');
        const data = await r.json();
        const s = typeof data.state === 'string' ? data.state : 'IDLE';
        bridge.setStateText?.(s);
        // timers are handled by /timers now
      } catch {}
    };
    poll();
    setInterval(poll, 350);
  }

  // ===== Lights mirror =====
  function setLamp(el, on) {
    if (!el) return;
    if (on) el.classList.add('on');
    else    el.classList.remove('on');
  }

  function startLightsPolling() {
    const poll = async () => {
      try {
        const r = await fetch(LIGHTS_ENDPOINT, { cache: 'no-store' });
        if (!r.ok) throw new Error('bad response');
        const data = await r.json();
        setLamp(road.red,    !!data.road?.red);
        setLamp(road.yellow, !!data.road?.yellow);
        setLamp(road.green,  !!data.road?.green);
        setLamp(ship.red,    !!data.boat?.red);
        setLamp(ship.yellow, !!data.boat?.yellow);
        setLamp(ship.green,  !!data.boat?.green);
      } catch (e) {
        // ignore
      }
    };
    poll();
    setInterval(poll, 250);
  }

  // ===== Timers (link ESP32 timing to timer chips) =====
  function startTimerPolling() {
    const poll = async () => {
      try {
        const r = await fetch(TIMERS_ENDPOINT, { cache: 'no-store' });
        if (!r.ok) throw new Error('bad response');
        const data = await r.json();

        const roadMs = (data.road && typeof data.road.remaining_ms === 'number')
          ? data.road.remaining_ms
          : 0;
        const boatMs = (data.boat && typeof data.boat.remaining_ms === 'number')
          ? data.boat.remaining_ms
          : 0;

        if (road.timer) {
          road.timer.textContent =
            roadMs > 0 ? (roadMs / 1000).toFixed(1) + 's' : '—';
        }
        if (ship.timer) {
          ship.timer.textContent =
            boatMs > 0 ? (boatMs / 1000).toFixed(1) + 's' : '—';
        }
      } catch (e) {
        // Optionally clear timers on error
        // if (road.timer) road.timer.textContent = '—';
        // if (ship.timer) ship.timer.textContent = '—';
      }
    };

    poll();
    setInterval(poll, 200);
  }
});
