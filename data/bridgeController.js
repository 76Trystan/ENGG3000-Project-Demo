class VerticalLiftBridgeController {
  constructor(rootElementId) {
    this.root = document.getElementById(rootElementId);
    this.manualEnabled = false;
    this.stateText = 'IDLE';
    this._renderUI();
  }

  setManualEnabled(enabled) {
    this.manualEnabled = enabled;
    const info = this.root.querySelector('#liftInfo');
    if (info) {
      info.textContent = enabled
        ? 'Controls enabled (Manual mode)'
        : 'Controls locked (Automatic mode)';
      info.style.color = enabled ? 'lightgreen' : '#cbd5e1';
    }
  }

  setStateText(t) {
    this.stateText = t || 'IDLE';
    const st = this.root.querySelector('#liftStatus');
    if (!st) return;
    st.textContent = this.stateText;

    let c = '#e5e7eb';
    if (t === 'OPENING') c = '#22c55e';
    else if (t === 'CLOSING') c = '#ef4444';
    else if (t === 'BOAT_WARNING' || t === 'ROAD_WARNING') c = '#f59e0b';
    st.style.color = c;
  }

  _renderUI() {
    this.root.innerHTML = `
      <div class="widget">
        <h3>Bridge Lift</h3>
        <p><strong>Status:</strong> <span id="liftStatus">IDLE</span></p>
        <p id="liftInfo" style="margin-top:6px;color:#cbd5e1">
          Controls locked (Automatic mode)
        </p>
      </div>
    `;
  }
}

export default VerticalLiftBridgeController;
