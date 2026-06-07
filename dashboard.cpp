#include "monitor.h"

const char* dashboardPage() {
  return R"PAGE(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MyLittleEnvironment</title>
<style>
  body { font-family: "New York Times"; font-size: 13px; color: #000; background: #fff; margin: 0; padding: 16px; }
  .wrap { max-width: 660px; margin: 0 auto; border: 1px solid #000; padding: 16px 18px; }
  h1 { font-size: 14px; letter-spacing: 2px; margin: 0 0 12px; padding-bottom: 8px; border-bottom: 2px solid #000; }
  .status { margin-bottom: 8px; font-weight: bold; }
  .status.active { color: #9a0000; }
  .notif { width: auto; margin: 6px 0 2px; }
  .notif th, .notif td { border: none; padding: 2px 18px 2px 0; text-align: left; font-weight: normal; width: auto; }
  table { border-collapse: collapse; width: 100%; margin-bottom: 14px; }
  th, td { border: 1px solid #000; padding: 6px 8px; }
  th { text-align: left; width: 45%; font-weight: normal; }
  td { text-align: right; }
  td.s { text-align: left; padding-left: 14px; }
  td.bad { color: #9a0000; font-weight: bold; }
  .range { margin-bottom: 6px; }
  .range label { margin: 0 4px 0 0; }
  .presets { margin-bottom: 10px; }
  .presets button { margin-right: 4px; }
  .note { font-size: 11px; margin: 4px 0 10px; }
  .clabel { font-size: 12px; font-weight: bold; margin: 12px 0 2px; }
  .stat { font-size: 11px; margin-bottom: 3px; }
  canvas { width: 100%; height: 110px; border: 1px solid #000; display: block; background: #fff; }
  fieldset { border: 1px solid #000; margin-top: 16px; padding: 10px 12px; }
  legend { font-weight: bold; padding: 0 6px; }
  .field { margin: 6px 0; }
  .field label { display: inline-block; width: 200px; }
  input { font-family: inherit; font-size: 12px; padding: 3px 4px; border: 1px solid #000; }
  input[type=number] { width: 84px; -moz-appearance: textfield; }
  input[type=number]::-webkit-outer-spin-button, input[type=number]::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
  button { padding: 3px 12px; border: 1px solid #000; background: #fff; cursor: pointer; font-family: inherit; font-size: 12px; }
</style>
</head>
<body>
<div class="wrap">
  <h1>My Little Environment</h1>

  <div id="status" class="status">--</div>

  <table>
    <tr><th>Temperature</th><td id="val_temp">--</td><td class="s" id="st_temp"></td></tr>
    <tr><th>Humidity</th><td id="val_humidity">--</td><td class="s" id="st_humidity"></td></tr>
    <tr><th>Carbon dioxide</th><td id="val_co2">--</td><td class="s" id="st_co2"></td></tr>
    <tr><th>Light intensity</th><td id="val_light">--</td><td class="s" id="st_light"></td></tr>
  </table>

  <div class="range">
    <label>From</label><input type="datetime-local" id="from" lang="en-GB">
    <label>To</label><input type="datetime-local" id="to" lang="en-GB">
    <button onclick="applyRange()">Show</button>
  </div>
  <div class="presets">
    <button onclick="preset(3600)">Last hour</button>
    <button onclick="preset(86400)">Last day</button>
    <button onclick="preset(604800)">Last week</button>
  </div>
  <div class="note">The dashed line is the average.</div>
  <div class="note" id="mem"></div>

  <div class="clabel">Temperature graph (C)</div>
  <div class="stat" id="stat_temp"></div>
  <canvas id="ch_temp"></canvas>

  <div class="clabel">Humidity graph (%)</div>
  <div class="stat" id="stat_humidity"></div>
  <canvas id="ch_humidity"></canvas>

  <div class="clabel">Carbon dioxide graph (PPM)</div>
  <div class="stat" id="stat_co2"></div>
  <canvas id="ch_co2"></canvas>

  <div class="clabel">Light intensity graph</div>
  <div class="stat" id="stat_light"></div>
  <canvas id="ch_light"></canvas>

  <fieldset>
    <legend>Desired ranges</legend>
    <div class="field"><label>Too cool below (C)</label><input id="thr_tempcool" type="number"><button onclick="setVal('tempcool')">Set</button></div>
    <div class="field"><label>Too warm above (C)</label><input id="thr_tempwarm" type="number"><button onclick="setVal('tempwarm')">Set</button></div>
    <div class="field"><label>Too dry below (%)</label><input id="thr_humiddry" type="number"><button onclick="setVal('humiddry')">Set</button></div>
    <div class="field"><label>Too humid above (%)</label><input id="thr_humidhumid" type="number"><button onclick="setVal('humidhumid')">Set</button></div>
    <div class="field"><label>CO2 low below (PPM)</label><input id="thr_co2low" type="number"><button onclick="setVal('co2low')">Set</button></div>
    <div class="field"><label>CO2 high above (PPM)</label><input id="thr_co2high" type="number"><button onclick="setVal('co2high')">Set</button></div>
    <div class="field"><label>Night below (0-1023)</label><input id="thr_night" type="number"><button onclick="setVal('night')">Set</button></div>
  </fieldset>

  <fieldset>
    <legend>Notification</legend>
    <div class="note">Get notified when a reading leaves its desired range.</div>
    <table class="notif">
      <tr><th></th><th>Buzzer</th><th>Telegram</th></tr>
      <tr><td>Temperature</td><td><input type="checkbox" id="bz_temp" onchange="setFlag('buzzer','temp')"></td><td><input type="checkbox" id="tg_temp" onchange="setFlag('telegram','temp')"></td></tr>
      <tr><td>Humidity</td><td><input type="checkbox" id="bz_humid" onchange="setFlag('buzzer','humid')"></td><td><input type="checkbox" id="tg_humid" onchange="setFlag('telegram','humid')"></td></tr>
      <tr><td>Carbon dioxide</td><td><input type="checkbox" id="bz_co2" onchange="setFlag('buzzer','co2')"></td><td><input type="checkbox" id="tg_co2" onchange="setFlag('telegram','co2')"></td></tr>
    </table>
  </fieldset>

  <fieldset>
    <legend>MQ135</legend>
    <div class="field"><button onclick="calibrate()">Calibrate in fresh air</button></div>
    <div class="note">Put the MQ135 sensor in fresh air, and please wait a few minutes to calibrate. </div>
  </fieldset>
</div>

<script>
var fromEpoch, toEpoch, liveEnd = true, liveSpan = 86400, deviceTime = 0, flagsSynced = false;

function pad(n) { return (n < 10 ? '0' : '') + n; }
function toInput(d) { return d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()) + 'T' + pad(d.getHours()) + ':' + pad(d.getMinutes()); }
function fmtClock(epoch) { var d = new Date(epoch * 1000); return d.getFullYear() + '/' + pad(d.getMonth() + 1) + '/' + pad(d.getDate()) + ' ' + pad(d.getHours()) + ':' + pad(d.getMinutes()); }

function liveNow() { return deviceTime || Math.floor(Date.now() / 1000); }

function setField(id, epoch) {
  var el = document.getElementById(id);
  if (document.activeElement !== el) el.value = toInput(new Date(epoch * 1000));
}
function showLiveWindow() {
  toEpoch = liveNow(); fromEpoch = toEpoch - liveSpan;
  setField('from', fromEpoch); setField('to', toEpoch);
}
function setDefaults() { liveSpan = 3600; liveEnd = true; showLiveWindow(); }
function preset(sec)   { liveSpan = sec;   liveEnd = true; showLiveWindow(); refresh(); }
function applyRange() {
  var f = new Date(document.getElementById('from').value).getTime();
  var t = new Date(document.getElementById('to').value).getTime();
  if (isNaN(f) || isNaN(t) || t <= f) return;
  fromEpoch = Math.floor(f / 1000); toEpoch = Math.floor(t / 1000);
  liveSpan = toEpoch - fromEpoch;
  liveEnd = (Math.floor(Date.now() / 1000) - toEpoch) < 120;
  refresh();
}
function setVal(key) {
  var v = document.getElementById('thr_' + key).value;
  if (v !== '') fetch('/set/' + key + '/' + v);
}
function calibrate() {
  fetch('/calibrate');
}
function setFlag(channel, key) {
  var box = document.getElementById((channel === 'buzzer' ? 'bz_' : 'tg_') + key);
  fetch('/set/' + channel + key + '/' + (box.checked ? 1 : 0));
}
function ph(id, v) { var e = document.getElementById(id); if (e) e.placeholder = v; }
function setCheck(id, on) { var e = document.getElementById(id); if (e) e.checked = !!on; }

function tlabel(epoch, span) {
  var d = new Date(epoch * 1000), hm = pad(d.getHours()) + ':' + pad(d.getMinutes());
  if (span >= 86400) return (d.getMonth() + 1) + '/' + d.getDate() + ' ' + hm;
  return hm;
}

function chart(key, times, values, dec, step, from, to) {
  var c = document.getElementById('ch_' + key), st = document.getElementById('stat_' + key);
  var ctx = c.getContext('2d'), dpr = window.devicePixelRatio || 1;
  var w = c.offsetWidth, h = 110;
  c.width = w * dpr; c.height = h * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);
  var n = values.length, i;
  var pad = 8, base = h - 18, span = (to - from) || 1;

  ctx.strokeStyle = '#000';
  ctx.beginPath(); ctx.moveTo(4, base); ctx.lineTo(w - 4, base); ctx.stroke();

  ctx.fillStyle = '#000'; ctx.font = '9px monospace'; ctx.textBaseline = 'top';
  ctx.fillText(tlabel(from, span), 4, base + 4);
  var last = tlabel(to, span);
  ctx.fillText(last, w - 4 - ctx.measureText(last).width, base + 4);
  ctx.textBaseline = 'alphabetic';

  if (n < 1) {
    if (st) st.textContent = '';
    ctx.font = '11px monospace';
    ctx.fillText('no data in this range', 10, h / 2);
    return;
  }

  var sum = 0, lo = values[0], hi = values[0];
  for (i = 0; i < n; i++) {
    sum += values[i];
    if (values[i] < lo) lo = values[i];
    if (values[i] > hi) hi = values[i];
  }
  var avg = sum / n;
  if (st) st.textContent = 'avg ' + avg.toFixed(dec) + '   min ' + lo.toFixed(dec) + '   max ' + hi.toFixed(dec);

  if (hi - lo < 1) { lo -= 1; hi += 1; }
  function Y(v) { return pad + (1 - (v - lo) / (hi - lo)) * (base - pad); }
  function X(i) { return 4 + (times[i] - from) / span * (w - 8); }

  ctx.strokeStyle = '#000'; ctx.setLineDash([4, 3]);
  ctx.beginPath(); ctx.moveTo(4, Y(avg)); ctx.lineTo(w - 4, Y(avg)); ctx.stroke();
  ctx.setLineDash([]);

  ctx.strokeStyle = '#000'; ctx.beginPath();
  for (i = 0; i < n; i++) {
    var x = X(i), y = Y(values[i]);
    var gap = i > 0 && step && (times[i] - times[i - 1]) > step * 2;
    if (i === 0 || gap) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
  if (n === 1) { ctx.fillStyle = '#000'; ctx.fillRect(X(0) - 2, Y(values[0]) - 2, 4, 4); }

  ctx.fillStyle = '#000'; ctx.font = '9px monospace';
  ctx.fillText(hi.toFixed(dec), 2, pad + 3);
  ctx.fillText(lo.toFixed(dec), 2, base - 2);
}

function statusClass(status) {
  if (status.indexOf('too') === 0) return 's bad';
  return 's';
}

function update(d) {
  if (d.time) deviceTime = d.time;
  var s = document.getElementById('status');
  if (d.notify) { s.textContent = d.notifyReason; s.className = 'status active'; }
  else { s.textContent = 'No buzzer or Telegram notifications.'; s.className = 'status'; }

  var mem = 'Holding ' + d.stored + ' / ' + d.capacity + ' samples';
  if (d.since) mem += ' since ' + fmtClock(d.since);
  document.getElementById('mem').textContent = mem;

  var L = d.limits;
  ph('thr_tempcool', L.tempCool); ph('thr_tempwarm', L.tempWarm);
  ph('thr_humiddry', L.humidDry); ph('thr_humidhumid', L.humidHumid);
  ph('thr_co2low', L.co2Low); ph('thr_co2high', L.co2High);
  ph('thr_night', L.night);

  if (!flagsSynced) {
    flagsSynced = true;
    setCheck('bz_temp', L.buzzerTemp); setCheck('bz_humid', L.buzzerHumid); setCheck('bz_co2', L.buzzerCO2);
    setCheck('tg_temp', L.telegramTemp); setCheck('tg_humid', L.telegramHumid); setCheck('tg_co2', L.telegramCO2);
  }

  d.readings.forEach(function(m) {
    document.getElementById('val_' + m.key).textContent = m.value + (m.unit ? ' ' + m.unit : '');
    var st = document.getElementById('st_' + m.key);
    st.textContent = m.status;
    st.className = statusClass(m.status);
    chart(m.key, d.times, m.history, m.decimals, d.step, fromEpoch, toEpoch);
  });
}

function refresh() {
  if (liveEnd) showLiveWindow();
  fetch('/api?from=' + fromEpoch + '&to=' + toEpoch).then(function(r) { return r.json(); }).then(update).catch(function() {});
}

setDefaults();
refresh();
setInterval(refresh, 3000);
</script>

Burak Can Arikan

</body>
</html>
)PAGE";
}