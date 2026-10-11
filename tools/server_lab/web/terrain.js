'use strict';
const terrainView = (() => {
  const get = id => document.getElementById(id);
  const palette = ['#55d8e2', '#f3ce75', '#ff8496', '#a995ff', '#70d9a5', '#ffad71', '#8eb8ff', '#f69ed8'];
  const npcStyles = new Map([
    [0, {name: '미니언', glyph: 'm', color: '#f3ce75', shape: 'triangle'}], [1, {name: '레드 드래곤', glyph: 'RD', color: '#ff6c80', shape: 'star'}],
    [2, {name: '그린 드래곤', glyph: 'GD', color: '#67df92', shape: 'star'}], [3, {name: '골렘', glyph: 'G', color: '#a4b4c9', shape: 'hex'}],
    [4, {name: '곰', glyph: 'B', color: '#d69b63', shape: 'square'}], [5, {name: '미노타우르', glyph: 'MT', color: '#c39aff', shape: 'pentagon'}],
    [6, {name: '상자', glyph: 'C', color: '#ffbc58', shape: 'chest'}], [7, {name: '비홀더', glyph: 'E', color: '#75c8ff', shape: 'eye'}],
    [100, {name: '상점', glyph: '$', color: '#55d8e2', shape: 'square'}], [101, {name: '경매', glyph: 'A', color: '#f3ce75', shape: 'square'}],
    [102, {name: '블록체인', glyph: 'BC', color: '#a995ff', shape: 'hex'}], [103, {name: '꾸미기', glyph: 'S', color: '#ff8496', shape: 'star'}]
  ]);
  const npcStyle = entity => npcStyles.get(entity.npc_type) ?? {name: '알 수 없음 ' + entity.npc_type, glyph: '?', color: '#94a0b0', shape: 'hex'};
  const actions = {spawn: '생성', attack: '공격 수신', remove: '제거', skill: '스킬 수신', 'skill-finish': '스킬 종료'};
  const cache = new Map(), motions = new Map();
  let peers = [], npcs = [], world = null, expectedRun = null, receivedAt = 0;
  let active = false, map = null, mapId = null, loadingId = null, failedId = null;
  let zoom = 1, panX = 0, panY = 0, drag = null, background = null, backgroundKey = '';
  let timer = null, polling = false, frame = null, lastFrame = 0;
  const format = value => Number.isFinite(value) ? value.toFixed(2) : '—';
  const group = peer => peer.group ?? Math.floor(peer.id / 4);
  const key = (entity, kind) => `${kind}:${group(entity)}:${entity.id}`;
  const groupMatches = entity => get('matchGroup').value === 'all' || group(entity) === Number(get('matchGroup').value);
  const selectedPeers = () => peers.filter(peer => peer.port === map?.port && Number.isFinite(peer.x) && groupMatches(peer));
  const selectedNpcs = () => map?.port === 8911 ? npcs.filter(groupMatches) : (map?.lobby_npcs?.npcs ?? []);
  const chosen =
      () => [...selectedPeers().map(e => [e, 'player']), ...selectedNpcs().map(e => [e, 'npc'])].find(([e, k]) => key(e, k) === get('followObject').value);
  const connectedNpc = entity => world?.status === 'running' && world.sources.some(source => source.group === group(entity) && source.observer !== null);
  const frozen = entity =>
      !entity.static && (world?.status !== 'running' || (entity.port ? !entity.connected : !connectedNpc(entity)) || Date.now() - receivedAt > 2000);
  const age = entity => Math.max(0, (Date.now() - (entity.position_ms ?? entity.updated_ms ?? Date.now())) / 1000);
  const recent = entity => Date.now() - (entity.action_ms ?? 0) < 1200 && ['attack', 'skill'].includes(entity.action);
  function motionLabel(entity) {
    if (entity.static)
      return '고정 배치 · 클라이언트';
    if (entity.removed)
      return '제거됨';
    if (frozen(entity))
      return '관측 종료 · 마지막 위치';
    if (recent(entity))
      return actions[entity.action];
    if (entity.idle === true)
      return '대기 응답';
    if (entity.speed > .01 && age(entity) < .8)
      return '이동 관측';
    return '위치 유지 / 수신 대기';
  }
  function rendered(entity, kind, at = performance.now()) {
    const motion = motions.get(key(entity, kind));
    if (!motion || frozen(entity) || entity.removed)
      return {x: entity.x, z: entity.z};
    const t = Math.min(1, Math.max(0, (at - motion.at) / 200));
    return {x: motion.x + (motion.toX - motion.x) * t, z: motion.z + (motion.toZ - motion.z) * t};
  }
  function transform(w, h) {
    const low = map.bounds.min, high = map.bounds.max;
    const scale = Math.min((w - 70) / (high[0] - low[0]), (h - 70) / (high[2] - low[2])) * zoom;
    return {scale, x: x => w / 2 + panX + (x - (low[0] + high[0]) / 2) * scale, y: z => h / 2 + panY - (z - (low[2] + high[2]) / 2) * scale};
  }
  function base(c, w, h, t) {
    c.fillStyle = '#0b1521';
    c.fillRect(0, 0, w, h);
    const low = map.bounds.min, high = map.bounds.max;
    c.save();
    c.beginPath();
    c.rect(8, 8, w - 16, h - 16);
    c.clip();
    c.drawImage(map.bitmap, t.x(low[0]), t.y(high[2]), (high[0] - low[0]) * t.scale, (high[2] - low[2]) * t.scale);
    if (get('navLayer').checked) {
      c.fillStyle = '#55d8e212';
      c.strokeStyle = '#55d8e231';
      c.lineWidth = .6;
      c.beginPath();
      for (const tri of map.nav_triangles) {
        c.moveTo(t.x(tri[0]), t.y(tri[1]));
        c.lineTo(t.x(tri[2]), t.y(tri[3]));
        c.lineTo(t.x(tri[4]), t.y(tri[5]));
        c.closePath();
      }
      c.fill();
      c.stroke();
    }
    const desired = 90 / t.scale, unit = 10 ** Math.floor(Math.log10(desired)), step = [1, 2, 5, 10].find(v => v * unit >= desired) * unit;
    c.font = '11px Segoe UI';
    c.strokeStyle = '#c0d5eb19';
    c.fillStyle = '#a7bdd0';
    c.lineWidth = 1;
    const x0 = (low[0] + high[0]) / 2 - (w / 2 + panX) / t.scale, z0 = (low[2] + high[2]) / 2 + (h / 2 + panY) / t.scale;
    for (let x = Math.ceil(x0 / step) * step; x < x0 + w / t.scale; x += step) {
      c.beginPath();
      c.moveTo(t.x(x), 0);
      c.lineTo(t.x(x), h);
      c.stroke();
      if (t.x(x) > 60 && t.x(x) < w - 65)
        c.fillText('X ' + format(x), t.x(x) + 3, h - 12);
    }
    for (let z = Math.floor(z0 / step) * step; z > z0 - h / t.scale; z -= step) {
      c.beginPath();
      c.moveTo(0, t.y(z));
      c.lineTo(w, t.y(z));
      c.stroke();
      if (t.y(z) > 18 && t.y(z) < h - 30)
        c.fillText('Z ' + format(z), 12, t.y(z) - 4);
    }
    c.restore();
  }
  function npcPath(c, x, y, shape) {
    const radius = 10;
    if (shape === 'eye') {
      c.ellipse(x, y, 12, 8, 0, 0, Math.PI * 2);
      return;
    }
    if (shape === 'square' || shape === 'chest') {
      c.rect(x - 10, y - (shape === 'chest' ? 7 : 10), 20, shape === 'chest' ? 14 : 20);
      return;
    }
    const points = shape === 'triangle' ? 3 : shape === 'pentagon' ? 5 : shape === 'star' ? 10 : 6;
    for (let i = 0; i < points; i++) {
      const angle = -Math.PI / 2 + i * Math.PI * 2 / points, r = shape === 'star' && i % 2 ? radius * .55 : radius;
      const px = x + Math.cos(angle) * r, py = y + Math.sin(angle) * r;
      if (i)
        c.lineTo(px, py);
      else
        c.moveTo(px, py);
    }
    c.closePath();
  }
  function marker(c, t, entity, kind) {
    const p = rendered(entity, kind), x = t.x(p.x), y = t.y(p.z), selected = key(entity, kind) === get('followObject').value;
    const style = npcStyle(entity);
    const color = kind === 'npc' ? style.color : frozen(entity) ? '#8290a3' : palette[entity.id % palette.length];
    const stamp = world?.status === 'running' ? Date.now() : Math.max(0, ...peers.map(e => e.position_ms ?? 0), ...npcs.map(e => e.position_ms ?? 0));
    const trail = (entity.trail ?? []).filter(point => point[0] >= stamp - 15000);
    if (get('trailLayer').checked && trail.length > 1) {
      c.strokeStyle = color;
      c.globalAlpha = kind === 'player' ? .7 : .3;
      c.lineWidth = kind === 'player' ? 2 : 1;
      c.beginPath();
      trail.forEach((point, i) => {
        if (i)
          c.lineTo(t.x(point[1]), t.y(point[2]));
        else
          c.moveTo(t.x(point[1]), t.y(point[2]));
      });
      c.stroke();
      c.globalAlpha = 1;
    }
    if (entity.removed) {
      if (Date.now() - (entity.action_ms ?? 0) < 3000 || selected) {
        c.strokeStyle = color;
        c.beginPath();
        c.moveTo(x - 5, y - 5);
        c.lineTo(x + 5, y + 5);
        c.moveTo(x + 5, y - 5);
        c.lineTo(x - 5, y + 5);
        c.stroke();
      }
      return;
    }
    let dx = entity.look_x ?? 0, dz = entity.look_z ?? 0;
    if (kind === 'player' && trail.length > 1 && entity.speed > .01) {
      const prev = trail[trail.length - 2];
      dx = entity.x - prev[1];
      dz = entity.z - prev[2];
    }
    const length = Math.hypot(dx, dz);
    if (length > .001) {
      dx = dx / length * 14;
      dz = -dz / length * 14;
      c.strokeStyle = color;
      c.lineWidth = 1.5;
      c.beginPath();
      c.moveTo(x, y);
      c.lineTo(x + dx, y + dz);
      c.lineTo(x + dx - dx * .3 - dz * .25, y + dz - dz * .3 + dx * .25);
      c.moveTo(x + dx, y + dz);
      c.lineTo(x + dx - dx * .3 + dz * .25, y + dz - dz * .3 - dx * .25);
      c.stroke();
    }
    if (recent(entity)) {
      c.strokeStyle = '#ff6d83';
      c.lineWidth = 2;
      c.beginPath();
      c.arc(x, y, 11 + (Date.now() - entity.action_ms) / 140, 0, Math.PI * 2);
      c.stroke();
    }
    c.fillStyle = color;
    c.strokeStyle = selected ? '#fff' : '#09121d';
    c.lineWidth = selected ? 2.5 : 1.5;
    c.beginPath();
    if (kind === 'player')
      c.arc(x, y, 6, 0, Math.PI * 2);
    else
      npcPath(c, x, y, style.shape);
    c.fill();
    c.stroke();
    if (kind === 'npc') {
      c.fillStyle = '#08121d';
      c.font = 'bold 9px Segoe UI';
      c.textAlign = 'center';
      c.textBaseline = 'middle';
      c.fillText(style.glyph, x, y);
      c.textAlign = 'start';
      c.textBaseline = 'alphabetic';
      c.fillStyle = color;
    }
    if (kind === 'player' || entity.static || selected || zoom >= 3) {
      const label = kind === 'player' ? '#' + entity.id : entity.static ? style.name : style.name + ' N' + entity.id;
      c.font = 'bold 11px Segoe UI';
      c.strokeStyle = '#07111b';
      c.lineWidth = 3;
      c.strokeText(label, x + 9, y - 8);
      c.fillText(label, x + 9, y - 8);
    }
    if (kind === 'npc' && entity.max_hp > 0) {
      c.fillStyle = '#2d3a48';
      c.fillRect(x - 8, y + 12, 16, 3);
      c.fillStyle = '#70d9a5';
      c.fillRect(x - 8, y + 12, 16 * Math.max(0, Math.min(1, entity.hp / entity.max_hp)), 3);
    }
  }
  function draw() {
    if (!active)
      return;
    const node = get('positions'), r = node.getBoundingClientRect(), dpr = window.devicePixelRatio || 1, w = r.width, h = r.height;
    const width = Math.max(1, Math.round(w * dpr)), height = Math.max(1, Math.round(h * dpr));
    if (node.width !== width || node.height !== height) {
      node.width = width;
      node.height = height;
    }
    const c = node.getContext('2d');
    c.setTransform(dpr, 0, 0, dpr, 0, 0);
    if (!map) {
      c.fillStyle = '#0b1521';
      c.fillRect(0, 0, w, h);
      return;
    }
    if (get('followCamera').checked) {
      const selected = chosen();
      if (selected) {
        const p = rendered(...selected), low = map.bounds.min, high = map.bounds.max, s = transform(w, h).scale;
        panX = -(p.x - (low[0] + high[0]) / 2) * s;
        panY = (p.z - (low[2] + high[2]) / 2) * s;
      }
    }
    const t = transform(w, h), signature = JSON.stringify([width, height, mapId, zoom, Math.round(panX), Math.round(panY), get('navLayer').checked]);
    if (signature !== backgroundKey) {
      base(c, w, h, t);
      background = document.createElement('canvas');
      background.width = width;
      background.height = height;
      background.getContext('2d').drawImage(node, 0, 0);
      backgroundKey = signature;
    } else {
      c.drawImage(background, 0, 0, w, h);
    }
    c.save();
    c.beginPath();
    c.rect(8, 8, w - 16, h - 16);
    c.clip();
    if (get('npcLayer').checked)
      for (const entity of selectedNpcs())
        marker(c, t, entity, 'npc');
    if (get('dummyLayer').checked)
      for (const entity of selectedPeers())
        marker(c, t, entity, 'player');
    c.restore();
    c.fillStyle = '#d5e2ef';
    c.font = '11px Segoe UI';
    c.fillText('+Z ↑  /  +X →', w - 110, 24);
    get('mapCount').textContent = `더미 ${selectedPeers().length}명 · NPC ${selectedNpcs().filter(e => !e.removed).length}개 · 확대 ${zoom.toFixed(1)}×`;
  }
  function row(root, values, selectKey = null, color = null) {
    const tr = document.createElement('tr');
    values.forEach((value, i) => {
      const td = document.createElement('td');
      if (i === 0 && selectKey) {
        const button = document.createElement('button');
        button.className = 'object-pick';
        button.textContent = value;
        button.onclick = () => selectObject(selectKey);
        td.append(button);
      } else
        td.textContent = value;
      tr.append(td);
    });
    if (color)
      tr.firstChild.style.color = color;
    root.append(tr);
  }
  function empty(root, columns, message) {
    if (!root.children.length) {
      const tr = document.createElement('tr'), td = document.createElement('td');
      td.colSpan = columns;
      td.className = 'empty';
      td.textContent = message;
      tr.append(td);
      root.append(tr);
    }
  }
  function legend() {
    const types = map?.port === 8910 ? [100, 101, 102, 103] : [0, 1, 2, 3, 4, 5, 6, 7], root = get('npcLegend'), signature = types.join(',');
    if (root.dataset.types === signature)
      return;
    root.replaceChildren();
    for (const type of types) {
      const style = npcStyles.get(type), item = document.createElement('span'), glyph = document.createElement('b');
      glyph.textContent = style.glyph;
      glyph.style.color = style.color;
      item.append(glyph, document.createTextNode(' ' + style.name));
      root.append(item);
    }
    root.dataset.types = signature;
  }
  function duration(ms) {
    const tenths = Math.round(Math.max(0, ms) / 100);
    return Math.floor(tenths / 600).toString().padStart(2, '0') + ':' + ((tenths % 600) / 10).toFixed(1).padStart(4, '0');
  }
  function schedules() {
    const root = get('matchTimers');
    root.replaceChildren();
    if (map?.port !== 8911) {
      root.textContent = '로비 NPC는 클라이언트 고정 배치입니다. 게임 일정은 게임 매치에서 표시됩니다.';
      return;
    }
    const samples = (world?.timelines ?? []).filter(groupMatches);
    if (!samples.length) {
      root.textContent = '서버의 매치 일정 수집 대기 · 계측을 켠 게임 서버와 게임 시나리오가 필요합니다.';
      return;
    }
    for (const sample of samples) {
      const card = document.createElement('div');
      card.className = 'timer-card';
      const heading = document.createElement('strong');
      const state = world.status !== 'running' ? '관측 종료' :
          sample.finished                      ? '게임 종료' :
          sample.stopped                       ? '서버 종료' :
          sample.stale                         ? '수집 지연 · 마지막 표본' :
          sample.live                          ? '실시간' :
                                                 '마지막 표본';
      heading.textContent = `매치 그룹 ${sample.group + 1} · 게임 ${duration(sample.game_display_ms)} · ${state}`;
      card.append(heading);
      const pending = ms => ms > 0 ? duration(ms) : '처리 대기';
      const rows = [
        ['자기장 게이트', sample.fence ? '해제까지 ' + pending(sample.fence_remaining_ms) : '해제됨 · 서버 확인'],
        ['다음 미니언 웨이브 검사', pending(sample.wave_remaining_ms) + ` · 활성 ${sample.active_minions}/12`],
        ['예약된 미니언 스폰', sample.spawns.length ? sample.spawns.map(e => `N${e.id} ${pending(e.remaining_ms)}`).join(' · ') : '현재 예약 없음']
      ];
      for (const [label, value] of rows) {
        const line = document.createElement('p'), title = document.createElement('span'), text = document.createElement('b');
        title.textContent = label;
        text.textContent = value;
        line.append(title, text);
        card.append(line);
      }
      root.append(card);
    }
  }
  function table() {
    const groups = [...new Set(peers.filter(e => e.port === 8911).map(group))].sort((a, b) => a - b), groupSignature = groups.join(',');
    if (get('matchGroup').dataset.options !== groupSignature) {
      const selected = get('matchGroup').value;
      get('matchGroup').replaceChildren(new Option('모든 매치', 'all'), ...groups.map(id => new Option('매치 그룹 ' + (id + 1), String(id))));
      get('matchGroup').value = groups.includes(Number(selected)) && selected !== 'all' ? selected : 'all';
      get('matchGroup').dataset.options = groupSignature;
    }
    get('matchGroup').disabled = map?.port !== 8911;
    const visiblePlayers = selectedPeers(), visibleNpcs = selectedNpcs();
    const options = [
      ...visiblePlayers.map(e => [key(e, 'player'), `더미 #${e.id} · 그룹 ${group(e) + 1}`]),
      ...visibleNpcs.map(e => [key(e, 'npc'), `NPC ${e.id} · ${npcStyle(e).name} · ${e.static ? '로비 고정' : '그룹 ' + (group(e) + 1)}`])
    ];
    const signature = JSON.stringify(options), select = get('followObject');
    if (select.dataset.options !== signature) {
      const old = select.value;
      select.replaceChildren(new Option('선택 없음', ''), ...options.map(([id, label]) => new Option(label, id)));
      select.value = options.some(([id]) => id === old) ? old : '';
      select.dataset.options = signature;
    }
    const players = get('positionRows');
    players.replaceChildren();
    for (const e of visiblePlayers)
      row(players,
          [
            '#' + e.id, e.player_id ?? '—', map.port === 8911 ? group(e) + 1 : '—', format(e.x), format(e.y), format(e.z),
            motionLabel(e) + (e.speed > .01 ? ' · ' + format(e.speed) + ' units/s' : '')
          ],
          key(e, 'player'), palette[e.id % palette.length]);
    empty(players, 7, '이 지형에서 수신한 더미 위치가 없습니다.');
    const npcRoot = get('npcRows');
    npcRoot.replaceChildren();
    for (const e of visibleNpcs)
      row(npcRoot,
          [
            `${npcStyle(e).glyph} · ${npcStyle(e).name} N${e.id}`, e.static ? '로비 고정' : group(e) + 1, `${format(e.x)} / ${format(e.y)} / ${format(e.z)}`,
            motionLabel(e), e.hp === undefined ? '—' : `${e.hp} / ${e.max_hp}`, e.static ? '—' : e.attacks
          ],
          key(e, 'npc'), npcStyle(e).color);
    empty(npcRoot, 6, map?.port === 8910 ? '로비에는 NPC 관측이 없습니다.' : '게임 시나리오를 실행하면 수신한 NPC가 나타납니다.');
    const events = get('npcEvents');
    events.replaceChildren();
    for (const e of [...(world?.events ?? [])].reverse().filter(e => map?.port === 8911 && groupMatches(e)).slice(0, 20))
      row(events, [new Date(e.timestamp_ms).toLocaleTimeString('ko-KR'), group(e) + 1, 'N' + e.npc_id, actions[e.action] ?? e.action]);
    empty(events, 4, '수신한 NPC 생성·공격·제거 사건이 없습니다.');
    const selected = chosen();
    get('selectedDetail').textContent = selected ? `${selected[1] === 'player' ? '더미 #' : 'NPC N'}${selected[0].id} · ${motionLabel(selected[0])} · X ${
                                                       format(selected[0].x)} / Y ${format(selected[0].y)} / Z ${format(selected[0].z)} · ${
                                                       selected[0].static ? '클라이언트 고정 좌표' : '위치 수신 ' + age(selected[0]).toFixed(1) + '초 전'}` :
                                                   '객체를 선택하면 위치·동작·수신 시각을 확인할 수 있습니다.';
    get('followCamera').disabled = !selected;
    legend();
    schedules();
  }
  function selectObject(value) {
    get('followObject').value = value;
    if (value && zoom < 3)
      zoom = 4;
    get('followCamera').checked = Boolean(value);
    table();
    draw();
  }
  async function load(id) {
    loadingId = id;
    failedId = null;
    map = null;
    mapId = id;
    zoom = 1;
    panX = panY = 0;
    backgroundKey = '';
    get('terrainStatus').textContent = '서버 지형 원본을 읽는 중…';
    get('terrainStatus').className = 'hint';
    draw();
    table();
    get('terrainSources').textContent = '';
    get('heightLegend').textContent = '';
    get('mapCount').textContent = '';
    if (!cache.has(id))
      cache.set(id, (async () => {
                  const response = await fetch('/api/terrain?map=' + encodeURIComponent(id)), data = await response.json();
                  if (!response.ok)
                    throw Error(data.error || '지형 로드 실패');
                  const bitmap = new Image();
                  bitmap.src = data.image;
                  await bitmap.decode();
                  return {...data, bitmap};
                })());
    try {
      const data = await cache.get(id);
      if (mapId !== id)
        return;
      map = data;
      get('terrainStatus').textContent =
          `${id === 'game' ? '게임' : '로비'} 지형 · 원본 삼각형 ${data.height.triangles.toLocaleString()}개 · ${data.resolution}² 높이 영상`;
      get('terrainSources').textContent = `${data.height.source} / ${data.navigation.source}`;
      get('heightLegend').textContent = `높이 Y ${format(data.bounds.min[1])} → ${format(data.bounds.max[1])}`;
      table();
      draw();
    } catch (error) {
      cache.delete(id);
      if (mapId !== id)
        return;
      failedId = id;
      get('terrainStatus').textContent = error.message + ' · 다시 불러오기를 누르세요';
      get('terrainStatus').className = 'error';
    } finally {
      if (loadingId === id)
        loadingId = null;
    }
  }
  function refresh() {
    if (!active)
      return;
    const id = get('terrainMap').value === 'auto' ? (peers.some(e => e.port === 8911) ? 'game' : 'lobby') : get('terrainMap').value;
    if (mapId !== id || (!map && loadingId !== id && failedId !== id))
      load(id);
    else {
      table();
      draw();
    }
  }
  async function pollWorld() {
    if (polling || !active)
      return;
    polling = true;
    try {
      const response = await fetch('/api/world'), data = await response.json();
      if (!response.ok)
        throw Error(data.error || '월드 관측 실패');
      if (data.run_id !== expectedRun)
        return;
      const at = performance.now(), liveKeys = new Set();
      for (const [entities, kind] of [[data.players, 'player'], [data.npcs, 'npc']])
        for (const entity of entities) {
          const id = key(entity, kind), old = motions.get(id), p = old ? rendered({...entity, connected: true}, kind, at) : {x: entity.x, z: entity.z};
          motions.set(id, {x: p.x, z: p.z, toX: entity.x, toZ: entity.z, at});
          liveKeys.add(id);
        }
      for (const id of motions.keys())
        if (!liveKeys.has(id))
          motions.delete(id);
      world = data;
      peers = data.players;
      npcs = data.npcs;
      receivedAt = Date.now();
      get('worldStatus').textContent =
          data.run_id ? (data.status === 'running' ? '● 월드 관측 중 · 0.2초 갱신' : '관측 종료 · 마지막 수신 상태') : '시나리오 실행 대기';
      get('worldStatus').className = 'hint ' + (data.status === 'running' ? 'live' : '');
      if (data.dropped) {
        get('worldStatus').textContent += ` · 저장 상한 초과 ${data.dropped}`;
        get('worldStatus').className = 'hint failed';
      }
      refresh();
    } catch (error) {
      get('worldStatus').textContent = '월드 관측 연결 끊김 · 재연결 중';
      get('worldStatus').className = 'hint failed';
    } finally {
      polling = false;
      clearTimeout(timer);
      if (active)
        timer = setTimeout(pollWorld, 200);
    }
  }
  function animate(at) {
    if (!active) {
      frame = null;
      return;
    }
    if (at - lastFrame >= 33) {
      draw();
      lastFrame = at;
    }
    frame = requestAnimationFrame(animate);
  }
  get('terrainMap').onchange = () => {
    failedId = null;
    get('followObject').value = '';
    get('followCamera').checked = false;
    refresh();
  };
  get('matchGroup').onchange = () => {
    get('followObject').value = '';
    get('followCamera').checked = false;
    table();
    draw();
  };
  for (const id of ['navLayer', 'dummyLayer', 'npcLayer', 'trailLayer', 'followCamera'])
    get(id).onchange = draw;
  get('followObject').onchange = () => selectObject(get('followObject').value);
  get('fitMap').onclick = () => {
    zoom = 1;
    panX = panY = 0;
    get('followCamera').checked = false;
    draw();
  };
  get('reloadMap').onclick = () => {
    if (mapId) {
      cache.delete(mapId);
      load(mapId);
    }
  };
  function scale(factor) {
    zoom = Math.max(1, Math.min(24, zoom * factor));
    draw();
  }
  get('zoomIn').onclick = () => scale(1.4);
  get('zoomOut').onclick = () => scale(1 / 1.4);
  const node = get('positions');
  node.addEventListener('wheel', event => {
    event.preventDefault();
    scale(event.deltaY < 0 ? 1.15 : 1 / 1.15);
  }, {passive: false});
  node.onpointerdown = event => {
    drag = {x: event.clientX, y: event.clientY};
    get('followCamera').checked = false;
    node.setPointerCapture(event.pointerId);
  };
  node.onpointermove = event => {
    if (!drag)
      return;
    panX += event.clientX - drag.x;
    panY += event.clientY - drag.y;
    drag = {x: event.clientX, y: event.clientY};
    draw();
  };
  node.onpointerup = node.onpointercancel = () => {
    drag = null;
  };
  window.addEventListener('resize', draw);
  return {
    update(run) {
      const id = run?.run_id ?? null;
      if (id !== expectedRun) {
        expectedRun = id;
        world = null;
        peers = [];
        npcs = [];
        motions.clear();
        get('followObject').value = '';
        get('followCamera').checked = false;
      }
      refresh();
    },
    activate(value) {
      active = value;
      clearTimeout(timer);
      if (active) {
        refresh();
        pollWorld();
        if (frame === null)
          frame = requestAnimationFrame(animate);
      } else if (frame !== null) {
        cancelAnimationFrame(frame);
        frame = null;
      }
    }
  };
})();
