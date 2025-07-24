/******************************************************************************
#
# Copyright (C) 2023 - 2028 KETI, All rights reserved.
#                           (Korea Electronics Technology Institute)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# Use of the Software is limited solely to applications:
# (a) running for Korean Government Project, or
# (b) that interact with KETI project/platform.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# KETI BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name of the KETI shall not be used
# in advertising or otherwise to promote the sale, use or other dealings in
# this Software without prior written authorization from KETI.
#
#******************************************************************************/

// mapboxConfigs를 전역에 선언 (window.onload, main 등 어디서든 접근 가능)
const mapboxConfigs = {
    pangyo: {
        accessToken: 'pk.eyJ1IjoieWVzYm1hbiIsImEiOiJjbHoxNHVydHQyNzBzMmpzMHNobGUxNnZ6In0.bAFH10On30d_Cj-zTMi53Q',
        style: 'mapbox://styles/yesbman/cm3mdtaea00bj01sq91q9egy8',
        center: [127.1021, 37.4064]
    },
    daegu: {
        accessToken: 'pk.eyJ1IjoieWVzYm1hbiIsImEiOiJjbHoxNHVydHQyNzBzMmpzMHNobGUxNnZ6In0.bAFH10On30d_Cj-zTMi53Q',
        style: 'mapbox://styles/yesbman/cm3pfcrd1000j01svb1deeqr0',
        center: [128.601763, 35.869757]
    },
    incheon: {
        accessToken: 'pk.eyJ1IjoieWVzYm1hbiIsImEiOiJjbHoxNHVydHQyNzBzMmpzMHNobGUxNnZ6In0.bAFH10On30d_Cj-zTMi53Q',
        style: 'mapbox://styles/yesbman/cm2t00fr600dl01r6hpir3mvk',
        center: [126.705206, 37.456256]
    },
    cheongju: {
        accessToken: 'pk.eyJ1IjoieWVzYm1hbiIsImEiOiJjbHoxNHVydHQyNzBzMmpzMHNobGUxNnZ6In0.bAFH10On30d_Cj-zTMi53Q',
        style: 'mapbox://styles/yesbman/cm3mlcdyu004o01rb9fjj58sx',
        center: [127.442150, 36.727757]
    },
    hwaseong: {
        accessToken: 'pk.eyJ1IjoieWVzYm1hbiIsImEiOiJjbHoxNHVydHQyNzBzMmpzMHNobGUxNnZ6In0.bAFH10On30d_Cj-zTMi53Q',
        style: 'mapbox://styles/yesbman/cm3pfcbzp000r01sn22n817n9',
        center: [126.771697, 37.239057]
    }
};

// PRR 그래프 개선을 위한 전역 변수들 (메모리 최적화 적용)
// 버퍼 재사용을 위한 관리 시스템
let prrDataBuffer = {
    x: null,
    y: null,
    index: 0,
    capacity: 0,
    ensureCapacity: function(size) {
        if (this.capacity < size) {
            // 기존 데이터 보존하면서 확장
            const newX = new Float32Array(size);
            const newY = new Float32Array(size);
            if (this.x && this.y) {
                newX.set(this.x.subarray(0, Math.min(this.index, this.capacity)));
                newY.set(this.y.subarray(0, Math.min(this.index, this.capacity)));
            }
            this.x = newX;
            this.y = newY;
            this.capacity = size;
        }
    },
    reset: function() {
        this.index = 0;
        // 버퍼는 유지하고 인덱스만 리셋
    },
    cleanup: function() {
        this.x = null;
        this.y = null;
        this.capacity = 0;
        this.index = 0;
    },
    
};
let rangeSize = 500; // 고정된 범위 크기
let isFollowingLatest = true; // 최신 데이터를 따라갈지 여부

// 메모리 풀링을 위한 전역 변수들 - 확장된 객체 풀링 시스템
const objectPools = {
    geoJson: {
        pool: [],
        maxSize: 1000,
        get: function() {
            if (this.pool.length > 0) {
                return this.pool.pop();
            }
            return {
                'type': 'Feature',
                'geometry': {
                    'type': 'Point',
                    'coordinates': [0, 0]
                },
                'properties': {}
            };
        },
        return: function(obj) {
            if (this.pool.length < this.maxSize) {
                // 객체 초기화
                obj.geometry.coordinates[0] = 0;
                obj.geometry.coordinates[1] = 0;
                obj.properties = {};
                this.pool.push(obj);
            }
        }
    },
    pathData: {
        pool: [],
        maxSize: 50,
        get: function(capacity) {
            for (let i = 0; i < this.pool.length; i++) {
                const item = this.pool[i];
                if (item.capacity >= capacity) {
                    this.pool.splice(i, 1);
                    item.gpsPathIndex = 0;
                    item.snappedPathIndex = 0;
                    return item;
                }
            }
            // 새로운 객체 생성
            return {
                gpsPathX: new Float32Array(capacity),
                gpsPathY: new Float32Array(capacity),
                gpsPathIndex: 0,
                snappedPathX: new Float32Array(capacity),
                snappedPathY: new Float32Array(capacity),
                snappedPathIndex: 0,
                capacity: capacity
            };
        },
        return: function(obj) {
            if (this.pool.length < this.maxSize && obj.capacity > 0) {
                // 인덱스만 리셋하고 버퍼는 유지
                obj.gpsPathIndex = 0;
                obj.snappedPathIndex = 0;
                this.pool.push(obj);
            }
        }
    }
};

// 레거시 함수 유지 (호환성)
function getGeoJsonObject() {
    return objectPools.geoJson.get();
}

function returnGeoJsonObject(obj) {
    objectPools.geoJson.return(obj);
}

// 그래프 버튼 및 평균값 계산을 위한 변수들
let prrValues = []; // PRR 값들을 저장하는 배열
let latencyValues = []; // Latency 값들을 저장하는 배열
let rssiValues = []; // RSSI 값들을 저장하는 배열
let rcpiValues = []; // RCPI 값들을 저장하는 배열

// DOM 요소 캐싱을 위한 전역 변수들
let domCache = {
    // 주요 요소들
    graphs: {},
    sensors: {},
    counts: {},
    buttons: {},
    lists: {},
    initialized: false
};

// DOM 캐시 초기화 함수
function initDomCache() {
    if (domCache.initialized) return;
    
    // 그래프 요소들
    domCache.graphs = {
        graph1: document.getElementById('graph1'),
        graph2: document.getElementById('graph2'),
        graph3: document.getElementById('graph3'),
        graph4: document.getElementById('graph4'),
        graphButtons: document.getElementById('graph-buttons')
    };
    
    // 센서 요소들
    domCache.sensors = {
        obuTxSensor: document.getElementById('obu-tx-sensor'),
        obuRxSensor: document.getElementById('obu-rx-sensor'),
        rsuSensor: document.getElementById('rsu-sensor')
    };
    
    // 카운트 요소들
    domCache.counts = {
        obuCount: document.getElementById('obu-count'),
        rsuCount: document.getElementById('rsu-count'),
        csvDataCount: document.getElementById('csv-data-count')
    };
    
    // 버튼 요소들
    domCache.buttons = {
        downloadCSV: document.getElementById('downloadCSVButton'),
        autoSave: document.getElementById('autoSaveButton')
    };
    
    // 리스트 요소들
    domCache.lists = {
        obuList: document.getElementById('obu-list'),
        rsuList: document.getElementById('rsu-list')
    };
    
    domCache.initialized = true;
}


// 리소스 해제 함수 - 확장된 메모리 정리
function clearResources() {
    // 타이머 정리
    if (window.uiUpdateTimer) {
        clearTimeout(window.uiUpdateTimer);
        window.uiUpdateTimer = null;
    }
    if (window.globalAutoSaveInterval) {
        clearInterval(window.globalAutoSaveInterval);
        window.globalAutoSaveInterval = null;
    }
    
    // WebSocket 연결 정리
    if (window.ws && window.ws.readyState === WebSocket.OPEN) {
        window.ws.close();
    }
    
    // PRR 버퍼 정리 (데이터는 유지, 참조만 정리)
    if (prrDataBuffer) {
        prrDataBuffer.cleanup();
    }
    
    // 이벤트 리스너 정리
    eventListeners.removeAll();
    
    // 타이머 정리
    timers.clearAll();
    
    // DOM 캠시 초기화
    domCache.initialized = false;
    domCache.graphs = {};
    domCache.sensors = {};
    domCache.counts = {};
    domCache.buttons = {};
    domCache.lists = {};
    
    // JSON 파싱 캠시 정리 (전역 변수 접근)
    try {
        if (typeof window.jsonParseCache !== 'undefined' && window.jsonParseCache) {
            window.jsonParseCache.clear();
        }
    } catch (e) {
        // 캠시 정리 실패 무시
    }
    
    // 객체 풀 정리
    try {
        if (typeof objectPools !== 'undefined') {
            objectPools.geoJson.pool = [];
            objectPools.pathData.pool = [];
        }
    } catch (e) {
        // 객체 풀 정리 실패 무시
    }
}

// 타이머 및 인터벌 관리
const timers = {
    intervals: new Set(),
    timeouts: new Set(),
    
    setInterval: function(callback, delay) {
        const id = setInterval(callback, delay);
        this.intervals.add(id);
        return id;
    },
    
    setTimeout: function(callback, delay) {
        const id = setTimeout(callback, delay);
        this.timeouts.add(id);
        return id;
    },
    
    clearInterval: function(id) {
        clearInterval(id);
        this.intervals.delete(id);
    },
    
    clearTimeout: function(id) {
        clearTimeout(id);
        this.timeouts.delete(id);
    },
    
    clearAll: function() {
        this.intervals.forEach(id => clearInterval(id));
        this.timeouts.forEach(id => clearTimeout(id));
        this.intervals.clear();
        this.timeouts.clear();
    }
};

// 이벤트 리스너 관리
const eventListeners = {
    listeners: new Map(),
    
    add: function(element, event, handler, options = false) {
        if (!element) return;
        
        const key = element.id || element.tagName + '_' + Math.random();
        if (!this.listeners.has(key)) {
            this.listeners.set(key, []);
        }
        
        this.listeners.get(key).push({ event, handler, options });
        element.addEventListener(event, handler, options);
    },
    
    removeAll: function() {
        this.listeners.forEach((eventList, key) => {
            const element = document.getElementById(key) || document.querySelector(`[data-key="${key}"]`);
            if (element) {
                eventList.forEach(({ event, handler, options }) => {
                    element.removeEventListener(event, handler, options);
                });
            }
        });
        this.listeners.clear();
    }
};

// 디바운스 함수 (과도한 호출 방지)
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// 쓰로틀 함수 (과도한 호출 방지)
function throttle(func, limit) {
    let inThrottle;
    return function() {
        const args = arguments;
        const context = this;
        if (!inThrottle) {
            func.apply(context, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    };
}


let isPrrGraphVisible = false; // PRR 그래프 표시 상태
let isLatencyGraphVisible = false; // Latency 그래프 표시 상태
let isRssiGraphVisible = false; // RSSI 그래프 표시 상태
let isRcpiGraphVisible = false; // RCPI 그래프 표시 상태

// 활성 장치 목록 관리를 위한 전역 변수들
let activeDevices = new Map(); // 장치 ID를 키로 하는 Map
const DEVICE_TIMEOUT = 10000; // 10초 동안 통신이 없으면 비활성으로 간주

// 선택된 장치 전역 변수
let selectedDevice = null;

// KD Tree 사용 여부를 장치별로 관리 (전역)
if (!window.deviceKdTreeUsage) {
    window.deviceKdTreeUsage = new Map();
}

// 장치별 경로 데이터 저장 (전역) - 메모리 효율적 관리
if (!window.devicePathData) {
    window.devicePathData = new Map();
}
const MAX_PATH_POINTS = 100000;

// 경로 데이터 메모리 관리자
const pathDataManager = {
    // 사용하지 않는 경로 데이터 정리
    cleanup: function() {
        const now = Date.now();
        for (const [deviceId, pathData] of window.devicePathData) {
            // 10분 이상 사용되지 않은 데이터 정리
            if (pathData.lastUsed && (now - pathData.lastUsed) > 600000) {
                objectPools.pathData.return(pathData);
                window.devicePathData.delete(deviceId);
            }
        }
    },
    // 압축 기능 - 사용하지 않는 부분의 메모리 정리
    compress: function(deviceId) {
        const pathData = window.devicePathData.get(deviceId);
        if (!pathData) return;
        
        const gpsCount = pathData.gpsPathIndex;
        const snappedCount = pathData.snappedPathIndex;
        const maxCount = Math.max(gpsCount, snappedCount);
        
        // 50% 이상 사용된 경우에만 압축
        if (maxCount < pathData.capacity * 0.5) {
            const newCapacity = Math.max(maxCount * 2, 1000);
            const newPathData = objectPools.pathData.get(newCapacity);
            
            // 데이터 복사
            newPathData.gpsPathX.set(pathData.gpsPathX.subarray(0, gpsCount));
            newPathData.gpsPathY.set(pathData.gpsPathY.subarray(0, gpsCount));
            newPathData.snappedPathX.set(pathData.snappedPathX.subarray(0, snappedCount));
            newPathData.snappedPathY.set(pathData.snappedPathY.subarray(0, snappedCount));
            newPathData.gpsPathIndex = gpsCount;
            newPathData.snappedPathIndex = snappedCount;
            newPathData.lastUsed = Date.now();
            
            // 기존 데이터 반환 및 새 데이터 설정
            objectPools.pathData.return(pathData);
            window.devicePathData.set(deviceId, newPathData);
        }
    }
};









// 전역 장치 관리 함수들
function getActiveDeviceByRole(role) {
    for (const [deviceId, device] of activeDevices) {
        if (device.type === 'OBU' && device.role === role && device.isActive) {
            return device;
        }
    }
    return null;
}

function getSelectedDeviceByRole(role) {
    if (selectedDevice && selectedDevice.type === 'OBU' && selectedDevice.role === role) {
        return selectedDevice;
    }
    return getActiveDeviceByRole(role);
}

function globalToggleDevicePathState(role) {
    const targetDevice = getSelectedDeviceByRole(role);
    if (targetDevice) {
        const currentKdTree = window.deviceKdTreeUsage.get(String(targetDevice.id)) || false;
        
        // globalToggleDevicePathState 로그 제거
        
        if (!targetDevice.isPathVisible) {
            // 상태 1: 비활성화 → 활성화 (빨간점)
            targetDevice.isPathVisible = true;
            window.deviceKdTreeUsage.set(String(targetDevice.id), false);
            // 상태 1 로그 제거
        } else if (targetDevice.isPathVisible && !currentKdTree) {
            // 상태 2: 활성화 → KD Tree 활성화 (파란점)
            targetDevice.isPathVisible = true;
            window.deviceKdTreeUsage.set(String(targetDevice.id), true);
            // 상태 2 로그 제거
        } else {
            // 상태 3: KD Tree 활성화 → 비활성화
            targetDevice.isPathVisible = false;
            window.deviceKdTreeUsage.set(String(targetDevice.id), false);
            // 상태 3 로그 제거
            
            // 완전한 소스 데이터 정리
            if (window.map) {
                const gpsSourceId = `gps-path-${targetDevice.id}`;
                const snappedSourceId = `snapped-path-${targetDevice.id}`;
                const gpsLayerId = `gps-path-layer-${targetDevice.id}`;
                const snappedLayerId = `snapped-path-layer-${targetDevice.id}`;
                
                try {
                    // 1. 소스 데이터 비우기 전에 기존 features 수집하여 메모리 정리
                    let gpsSource = window.map.getSource(gpsSourceId);
                    let snappedSource = window.map.getSource(snappedSourceId);
                    
                    // GPS 소스 데이터 정리
                    if (gpsSource) {
                        const gpsData = gpsSource._data;
                        if (gpsData && gpsData.features) {
                            // features 배열의 각 객체를 객체 풀에 반환
                            gpsData.features.forEach(feature => {
                                if (feature && typeof returnGeoJsonObject === 'function') {
                                    returnGeoJsonObject(feature);
                                }
                            });
                            // 배열 완전 정리
                            gpsData.features.length = 0;
                        }
                        // 빈 데이터로 설정
                        gpsSource.setData({
                            'type': 'FeatureCollection',
                            'features': []
                        });
                    }
                    
                    // 스냅된 소스 데이터 정리
                    if (snappedSource) {
                        const snappedData = snappedSource._data;
                        if (snappedData && snappedData.features) {
                            // features 배열의 각 객체를 객체 풀에 반환
                            snappedData.features.forEach(feature => {
                                if (feature && typeof returnGeoJsonObject === 'function') {
                                    returnGeoJsonObject(feature);
                                }
                            });
                            // 배열 완전 정리
                            snappedData.features.length = 0;
                        }
                        // 빈 데이터로 설정
                        snappedSource.setData({
                            'type': 'FeatureCollection',
                            'features': []
                        });
                    }
                    
                    // 2. 저장된 경로 데이터 정리 (pathData 객체 풀에 반환)
                    const deviceIdStr = String(targetDevice.id);
                    if (window.devicePathData && window.devicePathData.has(deviceIdStr)) {
                        const pathData = window.devicePathData.get(deviceIdStr);
                        if (pathData && typeof objectPools !== 'undefined' && objectPools.pathData) {
                            // pathData 객체를 풀에 반환
                            objectPools.pathData.return(pathData);
                        }
                        window.devicePathData.delete(deviceIdStr);
                    }
                    
                    // 3. 메모리 정리 함수 호출
                    if (typeof pathDataManager !== 'undefined' && pathDataManager.cleanup) {
                        pathDataManager.cleanup();
                    }
                    
                    // 4. 강제 가비지 컬렉션 힌트 (브라우저가 지원하는 경우)
                    if (window.gc && typeof window.gc === 'function') {
                        setTimeout(() => window.gc(), 100);
                    }
                    
                } catch (error) {
                    console.error(`소스 데이터 정리 중 오류 발생 - deviceId: ${targetDevice.id}`, error);
                }
            }
        }
        
        // 전역 updateAllDevicePaths 함수 호출
        if (typeof window.updateAllDevicePaths === 'function') {
            window.updateAllDevicePaths();
        }
        
        // 전역 updateDeviceControlButtons 함수 호출
        if (typeof window.updateDeviceControlButtons === 'function') {
            window.updateDeviceControlButtons(targetDevice);
        }
    } else {
        //console.log(`globalToggleDevicePathState - 대상 장치를 찾을 수 없음 (role: ${role})`);
    }
}

function globalToggleAutoTrack(role) {
    const targetDevice = getSelectedDeviceByRole(role);
    if (targetDevice) {
        if (targetDevice.isCentering) {
            // 전역 clearGlobalAutoTrack 함수 호출
            if (typeof window.clearGlobalAutoTrack === 'function') {
                window.clearGlobalAutoTrack();
            }
        } else {
            // 전역 setGlobalAutoTrack 함수 호출
            if (typeof window.setGlobalAutoTrack === 'function') {
                window.setGlobalAutoTrack(targetDevice);
            }
        }
    }
}

window.addEventListener('error', function(event) {
    console.error('JavaScript error occurred:', event.error || event.message || 'Unknown error');
});

window.onload = function() {
    // DOM 캠시 초기화
    initDomCache();
    
    // 그래프 버튼 초기 보기 설정 안보이는것으로 수정
    document.getElementById('graph1').style.display = 'none';
    document.getElementById('graph2').style.display = 'none';
    document.getElementById('graph3').style.display = 'none';
    document.getElementById('graph4').style.display = 'none';
    

    // 모달 창 열기
    document.getElementById('modal-background').style.display = 'block';
    document.getElementById('modal').style.display = 'block';

    let vehMode, CVehId, AVehId, vehicle0ImageUrl, vehicle1ImageUrl;

    let defaultIpAddress = "10.252.110.58";
    let testMode;
    let isTxTest;
    let VisiblePathMode;
    let isVisiblePath;
    let selectedRegion = 'pangyo'; // 기본값
    
    // 컨트롤 상태 변수들
    // 이제 장치별로 개별 관리됨
// let isCentering = false;
// let isPathVisible = false;

    // 전역 Auto Track 관리 변수
    let globalAutoTrackDevice = null;
    
    // 전역 통신선 관리 변수
    let globalCommunicationLineVisible = false;
    let communicationLineSource = null;
    let communicationLineLayer = null;
    
    // 장치별 통신선 활성화 상태 관리
    let deviceCommunicationLineStates = new Map(); // 장치 ID별 통신선 활성화 상태
    
    // 실제 통신 관계 추적을 위한 전역 변수
    let actualCommunicationPairs = new Set(); // 실제 통신이 발생한 장치 쌍들 (정규화된 형태)
    let lastCommunicationUpdate = 0; // 마지막 통신 업데이트 시간
    
    // 특정 장치의 경로 가시성 업데이트 함수 (성능 최적화)
    function updateDevicePathVisibility(deviceId, isVisible, useKdTree = false) {
        if (!window.map) return;
        
        const gpsLayerId = `gps-path-layer-${deviceId}`;
        const snappedLayerId = `snapped-path-layer-${deviceId}`;
        const gpsSourceId = `gps-path-${deviceId}`;
        const snappedSourceId = `snapped-path-${deviceId}`;
        
        //console.log(`updateDevicePathVisibility - deviceId: ${deviceId}, isVisible: ${isVisible}, useKdTree: ${useKdTree}`);
        
        if (isVisible) {
            // 저장된 경로 데이터로 소스 복원
            const pathData = window.devicePathData.get(String(deviceId));
            
            // GPS 소스가 없으면 생성
            if (!window.map.getSource(gpsSourceId)) {
                window.map.addSource(gpsSourceId, {
                    'type': 'geojson',
                    'data': {'type': 'FeatureCollection', 'features': []}
                });
            }
            
            // 스냅된 소스가 없으면 생성
            if (!window.map.getSource(snappedSourceId)) {
                window.map.addSource(snappedSourceId, {
                    'type': 'geojson',
                    'data': {'type': 'FeatureCollection', 'features': []}
                });
            }
            
            if (pathData) {
                // GPS 경로 데이터 복원
                if (pathData.gpsPathIndex > 0) {
                    const gpsFeatures = [];
                    for (let i = 0; i < pathData.gpsPathIndex; i++) {
                        gpsFeatures.push({
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': [pathData.gpsPathX[i], pathData.gpsPathY[i]]
                            },
                            'properties': { 'deviceId': deviceId }
                        });
                    }
                    window.map.getSource(gpsSourceId).setData({
                        'type': 'FeatureCollection',
                        'features': gpsFeatures
                    });
                }
                
                // 스냅된 경로 데이터 복원
                if (pathData.snappedPathIndex > 0) {
                    const snappedFeatures = [];
                    for (let i = 0; i < pathData.snappedPathIndex; i++) {
                        snappedFeatures.push({
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': [pathData.snappedPathX[i], pathData.snappedPathY[i]]
                            },
                            'properties': { 'deviceId': deviceId }
                        });
                    }
                    window.map.getSource(snappedSourceId).setData({
                        'type': 'FeatureCollection',
                        'features': snappedFeatures
                    });
                }
            }
            
            // GPS 레이어가 없으면 생성
            if (!window.map.getLayer(gpsLayerId)) {
                window.map.addLayer({
                    'id': gpsLayerId,
                    'type': 'circle',
                    'source': gpsSourceId,
                    'paint': {
                        'circle-radius': 3,
                        'circle-color': '#FF0000',
                    }
                });
                //console.log(`GPS 레이어 생성: ${gpsLayerId}`);
            }
            
            // 스냅된 레이어가 없으면 생성
            if (!window.map.getLayer(snappedLayerId)) {
                window.map.addLayer({
                    'id': snappedLayerId,
                    'type': 'circle',
                    'source': snappedSourceId,
                    'paint': {
                        'circle-radius': 3,
                        'circle-color': '#0000FF',
                    }
                });
                //console.log(`스냅된 레이어 생성: ${snappedLayerId}`);
            }
            
            // GPS 경로는 항상 표시 (KD Tree 사용 여부와 관계없이)
            window.map.setLayoutProperty(gpsLayerId, 'visibility', 'visible');
            //console.log(`GPS 경로 표시: ${gpsLayerId}`);
            
            // KD Tree 사용 여부에 따라 스냅된 경로 표시/숨김
            window.map.setLayoutProperty(snappedLayerId, 'visibility', useKdTree ? 'visible' : 'none');
            //console.log(`스냅된 경로 ${useKdTree ? '표시' : '숨김'}: ${snappedLayerId}`);
        } else {
            // 경로 완전 제거 (메모리 정리 포함)
            try {
                // 1. 소스 데이터 정리 전에 기존 features 수집하여 메모리 정리
                let gpsSource = window.map.getSource(gpsSourceId);
                let snappedSource = window.map.getSource(snappedSourceId);
                
                // GPS 소스 데이터 완전 정리
                if (gpsSource) {
                    const gpsData = gpsSource._data;
                    if (gpsData && gpsData.features) {
                        // features 배열의 각 객체를 객체 풀에 반환
                        gpsData.features.forEach(feature => {
                            if (feature && typeof returnGeoJsonObject === 'function') {
                                returnGeoJsonObject(feature);
                            }
                        });
                        // 배열 완전 정리
                        gpsData.features.length = 0;
                    }
                    // 빈 데이터로 설정
                    gpsSource.setData({
                        'type': 'FeatureCollection',
                        'features': []
                    });
                }
                
                // 스냅된 소스 데이터 완전 정리
                if (snappedSource) {
                    const snappedData = snappedSource._data;
                    if (snappedData && snappedData.features) {
                        // features 배열의 각 객체를 객체 풀에 반환
                        snappedData.features.forEach(feature => {
                            if (feature && typeof returnGeoJsonObject === 'function') {
                                returnGeoJsonObject(feature);
                            }
                        });
                        // 배열 완전 정리
                        snappedData.features.length = 0;
                    }
                    // 빈 데이터로 설정
                    snappedSource.setData({
                        'type': 'FeatureCollection',
                        'features': []
                    });
                }
                
                // 2. 레이어 먼저 제거
                if (window.map.getLayer(gpsLayerId)) {
                    window.map.removeLayer(gpsLayerId);
                }
                if (window.map.getLayer(snappedLayerId)) {
                    window.map.removeLayer(snappedLayerId);
                }
                
                // 3. 소스 나중에 제거
                if (window.map.getSource(gpsSourceId)) {
                    window.map.removeSource(gpsSourceId);
                }
                if (window.map.getSource(snappedSourceId)) {
                    window.map.removeSource(snappedSourceId);
                }
                
                // 4. 저장된 경로 데이터 완전 정리 (pathData 객체 풀에 반환)
                const deviceIdStr = String(deviceId);
                if (window.devicePathData.has(deviceIdStr)) {
                    const pathData = window.devicePathData.get(deviceIdStr);
                    if (pathData && typeof objectPools !== 'undefined' && objectPools.pathData) {
                        // pathData 객체를 풀에 반환
                        objectPools.pathData.return(pathData);
                    }
                    window.devicePathData.delete(deviceIdStr);
                }
                
                // 5. 메모리 정리 함수 호출
                if (typeof pathDataManager !== 'undefined' && pathDataManager.cleanup) {
                    pathDataManager.cleanup();
                }
                
                // 6. 강제로 지도 리렌더링
                window.map.triggerRepaint();
                
                // 7. 강제 가비지 컬렉션 힌트 (브라우저가 지원하는 경우)
                if (window.gc && typeof window.gc === 'function') {
                    setTimeout(() => window.gc(), 100);
                }
                
            } catch (error) {
                console.error(`경로 제거 중 오류 발생 - deviceId: ${deviceId}`, error);
            }
        }
    }
    
    // 모든 활성 장치의 경로 가시성 업데이트 함수 (전역)
    function updateAllDevicePaths() {
        if (!window.map) return;
        
        // updateAllDevicePaths 로그 제거
        
        // 모든 활성 장치를 확인하여 개별적으로 경로 상태 업데이트
        for (const [deviceId, device] of activeDevices) {
            const useKdTree = window.deviceKdTreeUsage.get(String(deviceId)) || false;
            // 장치별 경로 상태 로그 제거
            updateDevicePathVisibility(deviceId, device.isPathVisible, useKdTree);
        }
    }
    
    // 특정 장치의 경로 데이터를 완전히 제거하는 함수
    function clearDevicePathData(deviceId) {
        if (!window.map) return;
        
        const gpsSourceId = `gps-path-${deviceId}`;
        const snappedSourceId = `snapped-path-${deviceId}`;
        const gpsLayerId = `gps-path-layer-${deviceId}`;
        const snappedLayerId = `snapped-path-layer-${deviceId}`;
        const deviceIdStr = String(deviceId);
        
        try {
            // 1. 저장된 경로 데이터 완전 정리 (pathData 객체 풀에 반환)
            if (window.devicePathData.has(deviceIdStr)) {
                const pathData = window.devicePathData.get(deviceIdStr);
                if (pathData && typeof objectPools !== 'undefined' && objectPools.pathData) {
                    // pathData 객체를 풀에 반환
                    objectPools.pathData.return(pathData);
                }
                window.devicePathData.delete(deviceIdStr);
            }
            
            // 2. 소스 데이터 완전 정리
            let gpsSource = window.map.getSource(gpsSourceId);
            let snappedSource = window.map.getSource(snappedSourceId);
            
            // GPS 소스 데이터 완전 정리
            if (gpsSource) {
                const gpsData = gpsSource._data;
                if (gpsData && gpsData.features) {
                    // features 배열의 각 객체를 객체 풀에 반환
                    gpsData.features.forEach(feature => {
                        if (feature && typeof returnGeoJsonObject === 'function') {
                            returnGeoJsonObject(feature);
                        }
                    });
                    // 배열 완전 정리
                    gpsData.features.length = 0;
                }
                // 빈 데이터로 설정
                gpsSource.setData({
                    'type': 'FeatureCollection',
                    'features': []
                });
            }
            
            // 스냅된 소스 데이터 완전 정리
            if (snappedSource) {
                const snappedData = snappedSource._data;
                if (snappedData && snappedData.features) {
                    // features 배열의 각 객체를 객체 풀에 반환
                    snappedData.features.forEach(feature => {
                        if (feature && typeof returnGeoJsonObject === 'function') {
                            returnGeoJsonObject(feature);
                        }
                    });
                    // 배열 완전 정리
                    snappedData.features.length = 0;
                }
                // 빈 데이터로 설정
                snappedSource.setData({
                    'type': 'FeatureCollection',
                    'features': []
                });
            }
            
            // 3. 레이어 숨김
            if (window.map.getLayer(gpsLayerId)) {
                window.map.setLayoutProperty(gpsLayerId, 'visibility', 'none');
            }
            
            if (window.map.getLayer(snappedLayerId)) {
                window.map.setLayoutProperty(snappedLayerId, 'visibility', 'none');
            }
            
            // 4. 메모리 정리 함수 호출
            if (typeof pathDataManager !== 'undefined' && pathDataManager.cleanup) {
                pathDataManager.cleanup();
            }
            
            // 5. 강제 가비지 컬렉션 힌트 (브라우저가 지원하는 경우)
            if (window.gc && typeof window.gc === 'function') {
                setTimeout(() => window.gc(), 50);
            }
            
        } catch (error) {
            console.error(`clearDevicePathData 중 오류 발생 - deviceId: ${deviceId}`, error);
        }
    }
    
    // 디버깅을 위한 함수 - 현재 지도의 모든 경로 관련 소스와 레이어 확인
    function debugMapPathData() {
        if (!window.map) {
            //console.log('지도가 초기화되지 않았습니다.');
            return;
        }
        
        //console.log('=== 현재 지도의 경로 관련 소스와 레이어 상태 ===');
        
        // 모든 소스 확인
        const style = window.map.getStyle();
        if (style && style.sources) {
            //console.log('📊 소스 목록:');
            Object.keys(style.sources).forEach(sourceId => {
                if (sourceId.includes('gps-path') || sourceId.includes('snapped-path')) {
                    const source = window.map.getSource(sourceId);
                    if (source && source._data) {
                        const featureCount = source._data.features ? source._data.features.length : 0;
                        //console.log(`  - ${sourceId}: ${featureCount}개 점`);
                    }
                }
            });
        }
        
        // 모든 레이어 확인
        if (style && style.layers) {
            //console.log('🎨 레이어 목록:');
            style.layers.forEach(layer => {
                if (layer.id.includes('gps-path-layer') || layer.id.includes('snapped-path-layer')) {
                    const visibility = window.map.getLayoutProperty(layer.id, 'visibility');
                    //console.log(`  - ${layer.id}: ${visibility || 'visible'}`);
                }
            });
        }
        
        // 활성 장치 경로 상태 확인
        //console.log('🔧 활성 장치 경로 상태:');
        for (const [deviceId, device] of activeDevices) {
            const useKdTree = window.deviceKdTreeUsage.get(String(deviceId)) || false;
            //console.log(`  - 장치 ${deviceId}: isPathVisible=${device.isPathVisible}, useKdTree=${useKdTree}, isActive=${device.isActive}`);
        }
        
        // 저장된 경로 데이터 확인
        //console.log('💾 저장된 경로 데이터:');
        for (const [deviceId, pathData] of window.devicePathData) {
            //console.log(`  - 장치 ${deviceId}: GPS 경로 ${pathData.gpsPathIndex}개, 스냅된 경로 ${pathData.snappedPathIndex}개`);
        }
        
        //console.log('=== 디버깅 정보 출력 완료 ===');
    }
    
    // 전역 함수들 노출
    window.updateAllDevicePaths = updateAllDevicePaths;
    window.clearDevicePathData = clearDevicePathData;
    window.debugMapPathData = debugMapPathData;
    
    // 특정 장치의 경로 가시성 토글 함수 (전역)
    function toggleDevicePathVisibility(device) {
        updateAllDevicePaths();
    }
    
    // 장치별 경로 상태 관리 함수들 (전역)
    function setDevicePathState(deviceId, isVisible, useKdTree = false) {
        const device = activeDevices.get(String(deviceId));
        if (device) {
            // setDevicePathState 로그 제거
            device.isPathVisible = isVisible;
            window.deviceKdTreeUsage.set(String(deviceId), useKdTree);
            
            updateDevicePathVisibility(deviceId, isVisible, useKdTree);
            updateDeviceControlButtons(device);
        } else {
            // setDevicePathState 장치를 찾을 수 없음 로그 제거
        }
    }
    
    function toggleDevicePathState(deviceId) {
        const device = activeDevices.get(String(deviceId));
        if (!device) {
            // toggleDevicePathState 장치를 찾을 수 없음 로그 제거
            return;
        }
        
        const currentKdTree = window.deviceKdTreeUsage.get(String(deviceId)) || false;
        
        // toggleDevicePathState 로그 제거
        
        if (!device.isPathVisible) {
            // 상태 1: 비활성화 → 활성화 (빨간점)
            // 상태 1 로그 제거
            setDevicePathState(deviceId, true, false);
        } else if (device.isPathVisible && !currentKdTree) {
            // 상태 2: 활성화 → KD Tree 활성화 (파란점)
            // 상태 2 로그 제거
            setDevicePathState(deviceId, true, true);
        } else {
            // 상태 3: KD Tree 활성화 → 비활성화
            // 상태 3 로그 제거
            setDevicePathState(deviceId, false, false);
        }
    }
    
    // 전역 Auto Track 관리 함수
    function setGlobalAutoTrack(device) {
        // 이전 Auto Track 장치가 있으면 비활성화
        if (globalAutoTrackDevice && globalAutoTrackDevice !== device) {
            globalAutoTrackDevice.isCentering = false;
            updateDeviceControlButtons(globalAutoTrackDevice);
        }
        
        // 새로운 장치를 전역 Auto Track으로 설정
        globalAutoTrackDevice = device;
        device.isCentering = true;
        updateDeviceControlButtons(device);
    }
    
    // 전역 Auto Track 해제 함수
    function clearGlobalAutoTrack() {
        if (globalAutoTrackDevice) {
            globalAutoTrackDevice.isCentering = false;
            updateDeviceControlButtons(globalAutoTrackDevice);
            globalAutoTrackDevice = null;
        }
    }
    
    // 전역 함수들 노출
    window.setGlobalAutoTrack = setGlobalAutoTrack;
    window.clearGlobalAutoTrack = clearGlobalAutoTrack;
    
    // 장치 제어 버튼 상태 업데이트 함수
    function updateDeviceControlButtons(device) {
        if (!device || device.type !== 'OBU') return;
        
        const role = device.role;
        const useKdTree = window.deviceKdTreeUsage.get(String(device.id)) || false;
        
        // updateDeviceControlButtons 로그 제거
        
        if (role === 'Transmitter') {
            const autoTrackBtn = document.getElementById('obu-tx-auto-track');
            const visiblePathBtn = document.getElementById('obu-tx-visible-path');
            
            if (autoTrackBtn) {
                autoTrackBtn.classList.toggle('active', device.isCentering || false);
            }
            
            if (visiblePathBtn) {
                if (device.isPathVisible) {
                    if (useKdTree) {
                        visiblePathBtn.classList.remove('active');
                        visiblePathBtn.classList.add('active-kdtree');
                        // TX 버튼 상태 로그 제거
                    } else {
                        visiblePathBtn.classList.remove('active-kdtree');
                        visiblePathBtn.classList.add('active');
                        // TX 버튼 상태 로그 제거
                    }
                } else {
                    visiblePathBtn.classList.remove('active', 'active-kdtree');
                    // TX 버튼 상태 로그 제거
                }
            }
        } else if (role === 'Receiver') {
            const autoTrackBtn = document.getElementById('obu-rx-auto-track');
            const visiblePathBtn = document.getElementById('obu-rx-visible-path');
            
            if (autoTrackBtn) {
                autoTrackBtn.classList.toggle('active', device.isCentering || false);
            }
            
            if (visiblePathBtn) {
                if (device.isPathVisible) {
                    if (useKdTree) {
                        visiblePathBtn.classList.remove('active');
                        visiblePathBtn.classList.add('active-kdtree');
                        // RX 버튼 상태 로그 제거
                    }
                    else {
                        visiblePathBtn.classList.remove('active-kdtree');
                        visiblePathBtn.classList.add('active');
                        // RX 버튼 상태 로그 제거
                    }
                } else {
                    visiblePathBtn.classList.remove('active', 'active-kdtree');
                    // RX 버튼 상태 로그 제거
                }
            }
        }
    }
    
    // 전역 함수 노출
    window.updateDeviceControlButtons = updateDeviceControlButtons;
    
    // 통신선 관리 함수들
    function toggleCommunicationLine(deviceId = null) {
        // toggleCommunicationLine 호출 (로그 제거)
        
        if (deviceId) {
            // 특정 장치의 통신선 상태 토글
            const currentState = deviceCommunicationLineStates.get(String(deviceId)) || false;
            deviceCommunicationLineStates.set(String(deviceId), !currentState);
            
            // 전역 상태 업데이트 (하나라도 켜져있으면 전역도 켜짐)
            globalCommunicationLineVisible = Array.from(deviceCommunicationLineStates.values()).some(state => state);
            
            if (globalCommunicationLineVisible) {
                showCommunicationLine();
            } else {
                hideCommunicationLine();
            }
            
            // 해당 장치의 버튼 상태만 업데이트
            updateCommunicationLineButtons(deviceId);
        } else {
            // 전역 토글 (기존 방식)
            globalCommunicationLineVisible = !globalCommunicationLineVisible;
            if (globalCommunicationLineVisible) {
                showCommunicationLine();
            } else {
                hideCommunicationLine();
            }
            
            // 모든 버튼 상태 업데이트
            updateCommunicationLineButtons();
        }
    }
    
    function showCommunicationLine() {
        if (!window.map) {
            //console.log('showCommunicationLine: 지도가 없음');
            return;
        }
        
        // 통신선 소스 생성
        if (!communicationLineSource) {
                    try {
            window.map.addSource('communication-line', {
                'type': 'geojson',
                'data': {
                    'type': 'FeatureCollection',
                    'features': []
                }
            });
            communicationLineSource = window.map.getSource('communication-line');
        } catch (error) {
            console.error('통신선 소스 생성 실패:', error);
        }
    }
        
        // 통신선 레이어 생성
        if (!communicationLineLayer) {
            // 통신선 레이어를 최상단에 배치 (다른 레이어들 위에 표시)
            const layerConfig = {
                'id': 'communication-line-layer',
                'type': 'line',
                'source': 'communication-line',
                'layout': {
                    'line-join': 'round',
                    'line-cap': 'round',
                    'visibility': 'visible'
                },
                'paint': {
                    'line-color': ['get', 'color'], // PRR 색상 적용
                    'line-width': 3,
                    'line-opacity': 1.0
                }
            };
            //위에 레이어 추가 (beforeId로 최상단 배치)
            window.map.addLayer(layerConfig);
            communicationLineLayer = true;
        } else {
            // 기존 레이어가 있으면 보이도록 설정
            window.map.setLayoutProperty('communication-line-layer', 'visibility', 'visible');
        }
        
        // 통신선 표시
        updateCommunicationLineData();
    }
    
    function hideCommunicationLine() {
        if (!window.map) return;
        
        // 통신선 레이어 숨김
        if (window.map.getLayer('communication-line-layer')) {
            window.map.setLayoutProperty('communication-line-layer', 'visibility', 'none');
        }
    }
    
    function updateCommunicationLineData() {
        if (!window.map || !globalCommunicationLineVisible) {
            //console.log('통신선 업데이트 건너뜀 - map 또는 globalCommunicationLineVisible이 false');
            return;
        }
        
        // 비활성 장치 통신 쌍 즉시 정리
        cleanupInactiveCommunicationPairs();
        
        const features = [];
        
        // 전역 activeDevices 사용
        const globalActiveDevices = window.activeDevices || activeDevices;
        
        // 활성 장치들 간의 통신선 생성
        const activeDeviceArray = Array.from(globalActiveDevices.values()).filter(device => device.isActive);
        
        // 선택된 장치들 확인 (로그 제거)
        const selectedTxDevice = getSelectedDeviceByRole('Transmitter');
        const selectedRxDevice = getSelectedDeviceByRole('Receiver');
        const selectedRsuDevice = getSelectedDeviceByRole('RSU');
        
        // 각 통신 쌍에 대해 통신선 생성 (정규화된 형태로 처리)
        actualCommunicationPairs.forEach(pairKey => {
            const [commType, devicePair] = pairKey.split(':');
            const [device1Id, device2Id] = devicePair.split('-');
            const device1 = globalActiveDevices.get(device1Id);
            const device2 = globalActiveDevices.get(device2Id);
            if (device1 && device2 && device1.isActive && device2.isActive &&
                device1.latitude && device1.longitude && device2.latitude && device2.longitude) {
                // PRR 값 가져오기
                const commPairKey = `${Math.min(device1.id, device2.id)}-${Math.max(device1.id, device2.id)}`;
                const prr = communicationPairPRR.get(commPairKey);
                const prrColor = (prr !== undefined && prr !== null && !isNaN(prr)) ? getPrrGrade(prr).color : '#FF0000';
                features.push({
                    'type': 'Feature',
                    'geometry': {
                        'type': 'LineString',
                        'coordinates': [
                            [device1.longitude, device1.latitude],
                            [device2.longitude, device2.latitude]
                        ]
                    },
                    'properties': {
                        'type': commType,
                        'from': device1.id,
                        'to': device2.id,
                        'prr': prr,
                        'color': prrColor
                    }
                });
            }
        });
        
        // 통신선 생성 완료 (로그 제거)
        
        // 소스 데이터 업데이트
        if (communicationLineSource) {
            // 더미 통신선 추가 코드 삭제 (features.length === 0일 때)
            // features가 비어있어도 아무 것도 추가하지 않음
            const dataToSet = {
                'type': 'FeatureCollection',
                'features': features
            };
            // 통신선 데이터 설정 (로그 제거)
            try {
                communicationLineSource.setData(dataToSet);
                // 통신선 소스 데이터 업데이트 완료 (로그 제거)
            } catch (error) {
                console.error('통신선 소스 데이터 업데이트 실패:', error);
            }
        }
        // features 생성 후
        //console.log('[COMM] features:', features);
        //console.log('[COMM] activeDevices:', Array.from((window.activeDevices||activeDevices).entries()));
        //console.log('[COMM] communicationPairPRR:', Array.from(communicationPairPRR.entries()));
        //console.log('[COMM] actualCommunicationPairs:', Array.from(actualCommunicationPairs));
    }
    
    function updateCommunicationLineButtons(deviceId = null) {
        if (deviceId) {
            // 특정 장치의 버튼만 업데이트
            const deviceState = deviceCommunicationLineStates.get(String(deviceId)) || false;
            
            // 현재 선택된 장치가 해당 장치인지 확인하고 버튼 업데이트
            if (selectedDevice && selectedDevice.id == deviceId) {
                if (selectedDevice.type === 'OBU') {
                    if (selectedDevice.role === 'Transmitter') {
                        const txCommBtn = document.getElementById('obu-tx-communication-line');
                        if (txCommBtn) {
                            txCommBtn.classList.toggle('active', deviceState);
                        }
                    } else if (selectedDevice.role === 'Receiver') {
                        const rxCommBtn = document.getElementById('obu-rx-communication-line');
                        if (rxCommBtn) {
                            rxCommBtn.classList.toggle('active', deviceState);
                        }
                    }
                } else if (selectedDevice.type === 'RSU') {
                    const rsuCommBtn = document.getElementById('rsu-communication-line');
                    if (rsuCommBtn) {
                        rsuCommBtn.classList.toggle('active', deviceState);
                    }
                }
            }
        } else {
            // 모든 버튼 업데이트 (기존 방식)
            const txCommBtn = document.getElementById('obu-tx-communication-line');
            const rxCommBtn = document.getElementById('obu-rx-communication-line');
            const rsuCommBtn = document.getElementById('rsu-communication-line');
            
            if (txCommBtn && selectedDevice && selectedDevice.type === 'OBU' && selectedDevice.role === 'Transmitter') {
                const txState = deviceCommunicationLineStates.get(String(selectedDevice.id)) || false;
                txCommBtn.classList.toggle('active', txState);
            }
            if (rxCommBtn && selectedDevice && selectedDevice.type === 'OBU' && selectedDevice.role === 'Receiver') {
                const rxState = deviceCommunicationLineStates.get(String(selectedDevice.id)) || false;
                rxCommBtn.classList.toggle('active', rxState);
            }
            if (rsuCommBtn && selectedDevice && selectedDevice.type === 'RSU') {
                const rsuState = deviceCommunicationLineStates.get(String(selectedDevice.id)) || false;
                rsuCommBtn.classList.toggle('active', rsuState);
            }
        }
    }
    
    // 실제 통신 쌍 기록 함수 (정규화된 형태로 저장)
    function recordCommunicationPair(device1Id, device2Id, communicationType) {
        // 장치 ID를 정규화하여 항상 작은 ID가 앞에 오도록 함
        const normalizedDevice1Id = Math.min(device1Id, device2Id);
        const normalizedDevice2Id = Math.max(device1Id, device2Id);
        const pairKey = `${communicationType}:${normalizedDevice1Id}-${normalizedDevice2Id}`;
        
        // 중복 방지를 위해 Set에 추가
        actualCommunicationPairs.add(pairKey);
        lastCommunicationUpdate = Date.now();
    }
    
    // 즉시 정리 함수 (비활성 장치가 포함된 통신 쌍 즉시 제거)
    function cleanupInactiveCommunicationPairs() {
        const globalActiveDevices = window.activeDevices || new Map();
        const activeDeviceIds = new Set(Array.from(globalActiveDevices.keys()));
        let removedCount = 0;
        
        // 비활성 장치가 포함된 통신 쌍 제거
        actualCommunicationPairs.forEach(pairKey => {
            const [commType, devicePair] = pairKey.split(':');
            const [device1Id, device2Id] = devicePair.split('-');
            
            // 장치가 비활성화되었거나 존재하지 않으면 통신 쌍 제거
            const device1 = globalActiveDevices.get(device1Id);
            const device2 = globalActiveDevices.get(device2Id);
            
            if (!device1 || !device2 || !device1.isActive || !device2.isActive) {
                actualCommunicationPairs.delete(pairKey);
                removedCount++;
            }
        });
        
        // 비활성 장치의 통신선 상태도 정리
        deviceCommunicationLineStates.forEach((state, deviceId) => {
            const device = globalActiveDevices.get(deviceId);
            if (!device || !device.isActive) {
                deviceCommunicationLineStates.delete(deviceId);
            }
        });
        
        // 전역 상태 업데이트 (하나라도 켜져있으면 전역도 켜짐)
        globalCommunicationLineVisible = Array.from(deviceCommunicationLineStates.values()).some(state => state);
        
        // 정리 완료 (로그 제거)
    }
    
    // 전역 함수 노출
    window.toggleCommunicationLine = toggleCommunicationLine;
    window.updateCommunicationLineData = updateCommunicationLineData;
    window.recordCommunicationPair = recordCommunicationPair;
    
    // 모든 장치의 경로를 숨기는 함수
    function hideAllDevicePaths() {
        if (window.map) {
            //console.log(`hideAllDevicePaths - 모든 경로 숨기기 시작`);
            
            // 모든 활성 장치의 경로를 숨기기
            for (const [deviceId, device] of activeDevices) {
                device.isPathVisible = false;
                updateDevicePathVisibility(deviceId, false, false);
            }
            
            // 기존 gps-path, snapped-path 레이어들 숨김
            const layers = window.map.getStyle().layers || [];
            layers.forEach(layer => {
                if (layer.id.includes('gps-path-layer') || 
                    layer.id.includes('snapped-path-layer') || 
                    layer.id === 'kd-tree-points-layer') {
                    window.map.setLayoutProperty(layer.id, 'visibility', 'none');
                    //console.log(`레이어 숨김: ${layer.id}`);
                }
            });
            
            //console.log(`hideAllDevicePaths - 모든 경로 숨기기 완료`);
        }
    }


    // 드롭다운 값 변경 시 지역 변수 갱신
    const regionSelect = document.getElementById('regionSelect');
    if (regionSelect) {
        regionSelect.addEventListener('change', (event) => {
            selectedRegion = event.target.value;
        });
    }

    let currentMainInstance = null;
    function runMainWithRegion(region) {
        if (currentMainInstance && currentMainInstance.cleanup) {
            currentMainInstance.cleanup();
        }
        currentMainInstance = main(isTxTest, document.getElementById('ipAddress').value || defaultIpAddress, region);
    }

    // 버튼 클릭 이벤트 처리
    document.getElementById('submit-button').onclick = function() {
        // 사용자 입력 값 가져오기
        let vehType = document.getElementById('vehType').value.toLowerCase();
        let testType = document.getElementById('testType').value.toLowerCase();
        let ipAddress = document.getElementById('ipAddress').value || defaultIpAddress;
        let visiblePath = document.getElementById('visiblePath').value.toLowerCase();

        if (vehType === "cv") {
            vehMode = "C-VEH";
            CVehId = 23120008;
            AVehId = 23120002;
            vehicle0ImageUrl = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq5.png';
            vehicle1ImageUrl = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq-electric.png';
        } else if (vehType === "av") {
            vehMode = "A-VEH";
            CVehId = 23120008;
            AVehId = 23120002;
            vehicle0ImageUrl = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq-electric.png';
            vehicle1ImageUrl = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq5.png';
        } else {
            vehMode = "C-VEH";
            CVehId = 23120008;
            AVehId = 23120002;
            vehicle0ImageUrl = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq5.png';
            vehicle1ImageUrl = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq-electric.png';
        }

        if (testType === "tx") {
            testMode = "Tx Test";
            isTxTest = true;
        } else if (testType === "rx") {
            testMode = "Rx Test";
            isTxTest = false;
        } else {
            testMode = "Tx Test";
            isTxTest = true;
        }

        if (visiblePath === "yes") {
            VisiblePathMode = "is Enabled";
            isVisiblePath = true;
        } else if (visiblePath === "no") {
            VisiblePathMode = "is Disabled";
            isVisiblePath = false;
        } else {
            VisiblePathMode = "is Disabled";
            isVisiblePath = false;
        }

        // 모달 닫기
        document.getElementById('modal-background').style.display = 'none';
        document.getElementById('modal').style.display = 'none';

        // alert 창으로 현재 테스트 모드와 IP 주소를 출력
        alert(`현재 선택된 설정: ${vehMode}, ${testMode}\n입력된 IP 주소: ${ipAddress}\nVisible Path ${VisiblePathMode}`);

        // 지역에 맞게 main 함수 실행
        runMainWithRegion(selectedRegion);
    };

    // 기존 버튼 기능들
    function setupButtons() {
        // 기존 AUTO TRACK, VISIBLE PATH 버튼 이벤트는 제거 (숨김 처리됨)
        
        // 장치 리스트 헤더 클릭 이벤트
        const obuHeaderButton = document.querySelector('.obu-header .device-list-header-button');
        const rsuHeaderButton = document.querySelector('.rsu-header .device-list-header-button');
        
        if (obuHeaderButton) {
            obuHeaderButton.onclick = function(e) {
                e.stopPropagation(); // 이벤트 버블링 방지
                const obuList = document.getElementById('obu-list');
                if (obuList) {
                    const isVisible = obuList.style.display !== 'none';
                    obuList.style.display = isVisible ? 'none' : 'block';
                }
            };
        }
        
        if (rsuHeaderButton) {
            rsuHeaderButton.onclick = function(e) {
                e.stopPropagation(); // 이벤트 버블링 방지
                const rsuList = document.getElementById('rsu-list');
                if (rsuList) {
                    const isVisible = rsuList.style.display !== 'none';
                    rsuList.style.display = isVisible ? 'none' : 'block';
                }
            };
        }
        
        // 센서 패널 OBU TX AUTO TRACK 버튼
        document.getElementById('obu-tx-auto-track').onclick = function() {
            globalToggleAutoTrack('Transmitter');
        };

        // Visible Path 버튼 설정 함수 (3단계 순환: 비활성화 → 활성화 → KD Tree 활성화)
        function setupVisiblePathButton(buttonId, role) {
            const button = document.getElementById(buttonId);
            if (!button) return;

            button.addEventListener('click', function(e) {
                globalToggleDevicePathState(role);
            });
        }
        
        // 센서 패널 OBU TX VISIBLE PATH 버튼
        setupVisiblePathButton('obu-tx-visible-path', 'Transmitter');

        // 센서 패널 OBU RX AUTO TRACK 버튼
        document.getElementById('obu-rx-auto-track').onclick = function() {
            globalToggleAutoTrack('Receiver');
        };

        // 센서 패널 OBU RX VISIBLE PATH 버튼
        setupVisiblePathButton('obu-rx-visible-path', 'Receiver');
        
        // 통신선 버튼들 (현재 선택된 장치의 ID로 개별 토글)
        document.getElementById('obu-tx-communication-line').onclick = function() {
            if (selectedDevice && selectedDevice.type === 'OBU' && selectedDevice.role === 'Transmitter') {
                toggleCommunicationLine(selectedDevice.id);
            }
        };
        
        document.getElementById('obu-rx-communication-line').onclick = function() {
            if (selectedDevice && selectedDevice.type === 'OBU' && selectedDevice.role === 'Receiver') {
                toggleCommunicationLine(selectedDevice.id);
            }
        };
        
        document.getElementById('rsu-communication-line').onclick = function() {
            if (selectedDevice && selectedDevice.type === 'RSU') {
                toggleCommunicationLine(selectedDevice.id);
            }
        };

        // PRR 그래프 버튼
        const prrButtonHeader = document.querySelector('#prr-button .graph-button-header');
        prrButtonHeader.onclick = function(e) {
            e.stopPropagation(); // 이벤트 버블링 방지
            isPrrGraphVisible = !isPrrGraphVisible;
            const graph1 = document.getElementById('graph1');
            
            if (isPrrGraphVisible) {
                graph1.style.display = 'block';
            } else {
                graph1.style.display = 'none';
            }
        };

        // Latency 그래프 버튼
        const latencyButtonHeader = document.querySelector('#latency-button .graph-button-header');
        latencyButtonHeader.onclick = function(e) {
            e.stopPropagation(); // 이벤트 버블링 방지
            isLatencyGraphVisible = !isLatencyGraphVisible;
            const graph2 = document.getElementById('graph2');
            
            if (isLatencyGraphVisible) {
                graph2.style.display = 'block';
            } else {
                graph2.style.display = 'none';
            }
        };

        // RSSI 그래프 버튼
        const rssiButtonHeader = document.querySelector('#rssi-button .graph-button-header');
        rssiButtonHeader.onclick = function(e) {
            e.stopPropagation(); // 이벤트 버블링 방지
            isRssiGraphVisible = !isRssiGraphVisible;
            const graph3 = document.getElementById('graph3');
            
            if (isRssiGraphVisible) {
                graph3.style.display = 'block';
            } else {
                graph3.style.display = 'none';
            }
        };

        // RCPI 그래프 버튼
        const rcpiButtonHeader = document.querySelector('#rcpi-button .graph-button-header');
        rcpiButtonHeader.onclick = function(e) {
            e.stopPropagation(); // 이벤트 버블링 방지
            isRcpiGraphVisible = !isRcpiGraphVisible;
            const graph4 = document.getElementById('graph4');
            
            if (isRcpiGraphVisible) {
                graph4.style.display = 'block';
            } else {
                graph4.style.display = 'none';
            }
        };
    }

    // 초기 버튼 설정
    setupButtons();

    function main(isTxTest, ipAddress, region) {
        // KETI Pangyo
        const cKetiPangyoLatitude = 37.4064;
        const cKetiPangyolongitude = 127.1021;

        // RSU Location
        const cPangyoRsuLatitude16 = 37.408940;
        const cPangyoRsuLongitude16 = 127.099630;

        const cPangyoRsuLatitude17 = 37.406510;
        const cPangyoRsuLongitude17 = 127.100833;

        const cPangyoRsuLatitude18 = 37.405160;
        const cPangyoRsuLongitude18 = 127.103842;

        const cPangyoRsuLatitude5 = 37.410938;
        const cPangyoRsuLongitude5 = 127.094749;

        const cPangyoRsuLatitude31 = 37.411751;
        const cPangyoRsuLongitude31 = 127.095019;

        var vehicleLatitude0 = 37.406380;
        var vehicleLongitude0 = 127.102701;

        var vehicleLatitude1 = 37.406402;
        var vehicleLongitude1 = 127.102532;

        let s_unRxDevId, s_nRxLatitude, s_nRxLongitude, s_unRxVehicleHeading, s_unRxVehicleSpeed;
        let s_unTxDevId, s_nTxLatitude, s_nTxLongitude, s_unTxVehicleHeading, s_unTxVehicleSpeed;
        let s_unPdr, s_ulLatencyL1, s_ulTotalPacketCnt, s_unSeqNum;
        let s_nTxAttitude, s_nRxAttitude;
        let s_usCommDistance, s_nRssi, s_ucRcpi;
        let s_usTxSwVerL1, s_usTxSwVerL2, s_usTxHwVerL1, s_usTxHwVerL2, s_usRxSwVerL1, s_usRxSwVerL2, s_usRxHwVerL1, s_usRxHwVerL2;

        // map 객체를 완전히 전역(window.map)으로 관리
        if (window.map) {
            window.map.remove();
        }
        // map 컨테이너가 항상 존재하도록 보장
        let mapContainer = document.getElementById('map');
        if (!mapContainer) {
            mapContainer = document.createElement('div');
            mapContainer.id = 'map';
            // 원하는 부모에 append (여기서는 body에 추가)
            document.body.appendChild(mapContainer);
        }
        mapContainer.innerHTML = '';
        const config = mapboxConfigs[region] || mapboxConfigs['pangyo'];
        mapboxgl.accessToken = config.accessToken;
        window.map = new mapboxgl.Map({
            container: 'map',
            style: config.style,
            center: config.center,
            zoom: 19,
            projection: 'globe'
        });
        const map = window.map;

        function updateV2VPath(pathId, marker) {
            const V2VCoordinates = [
                [vehicleLongitude0, vehicleLatitude0], //실시간 본인 차량
                [vehicleLongitude1, vehicleLatitude1]
            ];

            if (map.getSource(pathId)) {
                map.getSource(pathId).setData({
                    'type': 'Feature',
                    'geometry': {
                        'type': 'LineString',
                        'coordinates': V2VCoordinates
                    }
                });

                // 중간 지점 마커 업데이트
                const midPoint = [
                    (V2VCoordinates[0][0] + V2VCoordinates[1][0]) / 2,
                    (V2VCoordinates[0][1] + V2VCoordinates[1][1]) / 2
                ];
                if (marker) {
                    marker.setLngLat(midPoint).addTo(map);
                }
            }
        }

        function updateV2IPath(pathId, marker) {
            const MRsuCoordinate = [127.440227, 36.730164];
            const V2ICoordinates = [
                [vehicleLongitude0, vehicleLatitude0],  //실시간 차량 위치
                MRsuCoordinate //고정 좌표 (mRSU)
            ];

            if (map.getSource(pathId)) {
                map.getSource(pathId).setData({
                    'type': 'Feature',
                    'geometry': {
                        'type': 'LineString',
                        'coordinates': V2ICoordinates
                    }
                });

                const midPoint = [
                    (V2ICoordinates[0][0] + MRsuCoordinate[0]) / 2,
                    (V2ICoordinates[0][1] + MRsuCoordinate[1]) / 2
                ];

                // 마커 위치를 차량 위치에 맞게 업데이트
                if (marker) {
                    marker.setLngLat(midPoint).addTo(map);
                }
            }
        }

        let s_unTempTxCnt = 0;
        let isPathPlan = false; // KD Tree 기본값을 비활성화로 변경

        /************************************************************/
        /* 활성 장치 관리 기능 */
        /************************************************************/
        
        // 활성 장치 목록을 관리하는 Map과 타임아웃 상수
        const activeDevices = new Map();
        const DEVICE_TIMEOUT = 10000; // 10초
        
        // 전역 변수로 노출
        window.activeDevices = activeDevices;
        
        // 선택된 장치 정보는 전역에서 관리됨
        
        // 클릭 디바운싱을 위한 변수
        let isSelecting = false;
        /************************************************************/
        
        // 장치 정보 업데이트 함수
        // UI 업데이트 쓰로틀링을 위한 변수
        let uiUpdateTimer = null;
        
        function updateDeviceInfo(deviceId, deviceType, additionalInfo = {}) {
            if (!deviceId || deviceId === 'undefined' || deviceId === 'NaN') {
                return;
            }
            
            const now = Date.now();
            const deviceKey = String(deviceId);
            
            if (activeDevices.has(deviceKey)) {
                // 기존 장치 정보 업데이트
                const device = activeDevices.get(deviceKey);
                
                // 경로 상태 보존 (기존 값 저장) - 단, 명시적으로 false로 설정된 경우는 보존하지 않음
                const preservedPathVisible = device.isPathVisible;
                const preservedCentering = device.isCentering;
                
                // 기존 장치 업데이트 로그 제거
                
                device.lastSeen = now;
                device.isActive = true;
                
                // 실제 패킷 카운트가 있으면 사용, 없으면 +1 증가
                if (additionalInfo.realPacketCount !== undefined) {
                    device.packetCount = additionalInfo.realPacketCount;
                } else {
                    device.packetCount = (device.packetCount || 0) + 1;
                }
                
                // 추가 정보 업데이트 (realPacketCount와 경로 상태 관련 속성 제외)
                const { realPacketCount, isPathVisible, isCentering, ...otherInfo } = additionalInfo;
                Object.assign(device, otherInfo);
                
                // 경로 상태 보존 - 단, 명시적으로 설정된 경우는 그대로 유지
                if (additionalInfo.isPathVisible === undefined) {
                    device.isPathVisible = preservedPathVisible;
                }
                if (additionalInfo.isCentering === undefined) {
                    device.isCentering = preservedCentering;
                }
                
                // 장치 업데이트 완료 로그 제거
                
                // 선택된 장치가 업데이트된 경우 센서값 다시 표시
                if (selectedDevice && selectedDevice.id === deviceKey && selectedDevice.type === deviceType) {
                    selectedDevice = device; // 최신 정보로 업데이트
                    updateSensorValuesForSelectedDevice();
                }
            } else {
                // 새 장치 추가
                const { realPacketCount, ...otherInfo } = additionalInfo;
                const newDevice = {
                    id: deviceKey,
                    type: deviceType,
                    firstSeen: now,
                    lastSeen: now,
                    isActive: true,
                    packetCount: realPacketCount !== undefined ? realPacketCount : 1,
                    // 경로 상태 초기화 (기본값으로 설정)
                    isPathVisible: false,
                    isCentering: false,
                    ...otherInfo
                };
                activeDevices.set(deviceKey, newDevice);
                
                // KD Tree 사용 여부 초기화 (기본값으로 false)
                if (!window.deviceKdTreeUsage.has(deviceKey)) {
                    window.deviceKdTreeUsage.set(deviceKey, false);
                }
                
                // 새 장치 추가 로그 제거
            }
            
            // UI 업데이트를 500ms에 한 번으로 제한 (더 반응적으로)
            if (uiUpdateTimer) {
                clearTimeout(uiUpdateTimer);
            }
            uiUpdateTimer = setTimeout(() => {
                updateDeviceListUI();
            }, 500);
        }
        
        // 장치 목록 UI 업데이트 함수
        function updateDeviceListUI() {
            const obuListElement = document.getElementById('obu-list');
            const rsuListElement = document.getElementById('rsu-list');
            const obuCountElement = document.getElementById('obu-count');
            const rsuCountElement = document.getElementById('rsu-count');
            
            if (!obuListElement || !rsuListElement || !obuCountElement || !rsuCountElement) {
                return;
            }
            
            // 비활성 장치 체크
            checkInactiveDevices();
            
            const deviceArray = Array.from(activeDevices.values());
            const obuDevices = deviceArray.filter(device => device.type === 'OBU');
            const rsuDevices = deviceArray.filter(device => device.type === 'RSU');
            
            const activeObuCount = obuDevices.filter(device => device.isActive).length;
            const activeRsuCount = rsuDevices.filter(device => device.isActive).length;
            
            // OBU TX/RX 개수 계산
            const activeTxCount = obuDevices.filter(device => device.isActive && device.role === 'Transmitter').length;
            const activeRxCount = obuDevices.filter(device => device.isActive && device.role === 'Receiver').length;
            
            // 각 섹션 카운트 업데이트 (TX/RX 구분 표시)
            if (activeTxCount > 0 && activeRxCount > 0) {
                obuCountElement.textContent = `${activeObuCount}개 (TX:${activeTxCount}, RX:${activeRxCount})`;
            } else if (activeTxCount > 0) {
                obuCountElement.textContent = `${activeObuCount}개 (TX)`;
            } else if (activeRxCount > 0) {
                obuCountElement.textContent = `${activeObuCount}개 (RX)`;
            } else {
                obuCountElement.textContent = `${activeObuCount}개`;
            }
            rsuCountElement.textContent = `${activeRsuCount}개`;
            
            // OBU 섹션 업데이트 (활성화된 장치만)
            const activeObuDevices = obuDevices.filter(device => device.isActive);
            updateDeviceSection(obuListElement, activeObuDevices, '검색된 OBU 장치가 없습니다');
            
            // RSU 섹션 업데이트 (활성화된 장치만)
            const activeRsuDevices = rsuDevices.filter(device => device.isActive);
            updateDeviceSection(rsuListElement, activeRsuDevices, '검색된 RSU 장치가 없습니다');
            
            // 선택된 장치가 비활성화되었는지 체크
            if (selectedDevice && (!activeDevices.has(selectedDevice.id) || !activeDevices.get(selectedDevice.id).isActive)) {

                selectedDevice = null;
                // 원래 TX/RX 표시로 복원하기 위해 fetchAndUpdateGraph 다시 호출
                setTimeout(() => {
                    fetchAndUpdateGraph();
                }, 100);
            } else if (selectedDevice) {
                // 선택된 장치 UI 업데이트
                updateSelectedDeviceUI();
            }
        }
        
        function updateDeviceSection(sectionElement, devices, noDeviceMessage) {
            // "장치가 없습니다" 메시지 제거
            const noDevicesElement = sectionElement.querySelector('.no-devices');
            if (noDevicesElement) {
                noDevicesElement.remove();
            }
            
            if (devices.length === 0) {
                sectionElement.innerHTML = `<div class="no-devices">${noDeviceMessage}</div>`;
                return;
            }
            
            // 기존 장치 요소들 추적
            const existingDevices = new Set();
            const existingElements = sectionElement.querySelectorAll('.device-item');
            existingElements.forEach(el => {
                const deviceId = el.getAttribute('data-device-id');
                if (deviceId) existingDevices.add(deviceId);
            });
            
            // 정렬된 장치 배열 (최근 통신 순)
            const sortedDevices = devices.sort((a, b) => b.lastSeen - a.lastSeen);
            
            // 각 장치에 대해 DOM 업데이트 또는 생성
            sortedDevices.forEach(device => {
                const deviceId = `${device.type}-${device.id}`;
                let deviceElement = sectionElement.querySelector(`[data-device-id="${deviceId}"]`);
                
                const timeSinceLastSeen = Date.now() - device.lastSeen;
                const secondsAgo = Math.floor(timeSinceLastSeen / 1000);
                
                let timeText;
                if (secondsAgo < 1) {
                    timeText = '방금 전';
                } else {
                    timeText = `${secondsAgo}초 전`;
                }
                
                // OBU는 TX/RX 구분, RSU는 그대로
                let deviceTypeText;
                if (device.type === 'OBU') {
                    const roleText = device.role === 'Transmitter' ? 'TX' : 
                                   device.role === 'Receiver' ? 'RX' : '';
                    deviceTypeText = roleText ? `OBU-${roleText}` : 'OBU';
                } else {
                    deviceTypeText = 'RSU';
                }
                
                // OBU TX/RX 클래스 추가
                let deviceClass = `device-item ${device.type.toLowerCase()}`;
                if (device.type === 'OBU') {
                    if (device.role === 'Transmitter') {
                        deviceClass += ' tx';
                    } else if (device.role === 'Receiver') {
                        deviceClass += ' rx';
                    }
                }
                const statusClass = 'status-active';
                const statusText = '활성';
                
                if (deviceElement) {
                    // 기존 요소 실시간 업데이트 (값만 변경)
                    deviceElement.className = deviceClass;
                    
                    const packetElement = deviceElement.querySelector('.device-type');
                    const timeElement = deviceElement.querySelector('.last-seen');
                    const statusElement = deviceElement.querySelector('.status-indicator');
                    
                    if (packetElement) packetElement.textContent = `패킷: ${device.packetCount}개`;
                    if (timeElement) timeElement.textContent = timeText;
                    if (statusElement) {
                        statusElement.className = `status-indicator ${statusClass}`;
                        statusElement.textContent = statusText;
                    }
                    
                    // 클릭 이벤트가 없으면 추가 (이벤트 버블링 방지)
                    if (!deviceElement.hasAttribute('data-click-added')) {
                        deviceElement.addEventListener('click', (event) => {
                            event.stopPropagation();
                            event.preventDefault();
                            selectDevice(device);
                        });
                        deviceElement.style.cursor = 'pointer';
                        deviceElement.setAttribute('data-click-added', 'true');
                    }
                    
                    existingDevices.delete(deviceId);
                } else {
                    // 새 장치 요소 생성
                    deviceElement = document.createElement('div');
                    deviceElement.className = deviceClass;
                    deviceElement.setAttribute('data-device-id', deviceId);
                    
                    // CAN 상태 정보 생성
                    let canStatus = '';
                    if (device.type === 'OBU' && device.role === 'Transmitter') {
                        const canStatuses = [];
                        if (device.epsEn === 'Enabled') canStatuses.push('EPS');
                        if (device.accEn === 'Enabled') canStatuses.push('ACC');
                        if (device.aebEn === 'Enabled') canStatuses.push('AEB');
                        if (canStatuses.length > 0) {
                            canStatus = `<div class="can-status">CAN: ${canStatuses.join(', ')}</div>`;
                        }
                    }
                    
                    deviceElement.innerHTML = `
                        <div class="device-info">
                            <div class="device-id">${deviceTypeText}#${device.id}</div>
                            <div class="device-type">패킷: ${device.packetCount}개</div>
                            ${canStatus}
                        </div>
                        <div class="device-status">
                            <div class="last-seen">${timeText}</div>
                            <div class="status-indicator ${statusClass}">${statusText}</div>
                        </div>
                    `;
                    
                    // 클릭 이벤트 추가 (이벤트 버블링 방지)
                    deviceElement.addEventListener('click', (event) => {
                        event.stopPropagation();
                        event.preventDefault();
                        selectDevice(device);
                    });
                    deviceElement.style.cursor = 'pointer';
                    
                    sectionElement.appendChild(deviceElement);
                }
            });
            
            // 더 이상 존재하지 않는 장치 요소 제거
            existingDevices.forEach(deviceId => {
                const elementToRemove = sectionElement.querySelector(`[data-device-id="${deviceId}"]`);
                if (elementToRemove) {
                    elementToRemove.remove();
                }
            });
        }
        
        // 장치 선택 함수 (디바운싱 적용)
        function selectDevice(device) {
            // 중복 선택 방지
            if (isSelecting) {
                return;
            }
            
            isSelecting = true;
            
            // 이미 선택된 장치를 다시 클릭하면 선택 해제
            if (selectedDevice && selectedDevice.id === device.id && selectedDevice.type === device.type) {
                selectedDevice = null;
                // RX가 해제되면 그래프 숨김
                document.getElementById('graph-buttons').style.display = 'none';
                document.getElementById('graph1').style.display = 'none';
                document.getElementById('graph2').style.display = 'none';
                document.getElementById('graph3').style.display = 'none';
                document.getElementById('graph4').style.display = 'none';
                // 센서 패널 숨기기
                hideSensorPanels();
                // 경로 상태는 유지하고 UI만 업데이트
                updateAllDevicePaths();
                // 선택 해제 시 원래 TX/RX 표시로 복원
                updateSelectedDeviceUI();
                setTimeout(() => {
                    fetchAndUpdateGraph();
                    isSelecting = false; // 처리 완료
                }, 100);
            } else {
                selectedDevice = device;
                // RX 디바이스를 선택한 경우에만 그래프 표시
                if (device.type === 'OBU' && device.role === 'Receiver') {
                    document.getElementById('graph-buttons').style.display = 'block';
                } else {
                    document.getElementById('graph-buttons').style.display = 'none';
                    document.getElementById('graph1').style.display = 'none';
                    document.getElementById('graph2').style.display = 'none';
                    document.getElementById('graph3').style.display = 'none';
                    document.getElementById('graph4').style.display = 'none';
                }
                // 선택된 장치 타입에 맞는 센서 패널 표시
                showSensorPanel(device);
                // 선택된 장치 시각적 표시 업데이트
                updateSelectedDeviceUI();
                // 선택된 장치의 센서값으로 업데이트
                updateSensorValuesForSelectedDevice();
                // 경로 표시 상태 업데이트 (모든 장치의 경로 상태 확인)
                updateAllDevicePaths();
                // 짧은 시간 후 처리 완료
                setTimeout(() => {
                    isSelecting = false;
                }, 200);
            }
        }
        
        // 센서 패널 표시 함수
        function showSensorPanel(device) {
            // 모든 센서 패널 숨기기
            hideSensorPanels();
            // 장치 타입에 따라 해당 센서 패널 표시
            if (device.type === 'OBU') {
                if (device.role === 'Transmitter') {
                    document.getElementById('obu-tx-sensor').style.display = 'block';
                    updateDeviceControlButtons(device);
                    updateCommunicationLineButtons(device.id);
                    updateAllDevicePaths();
                } else if (device.role === 'Receiver') {
                    document.getElementById('obu-rx-sensor').style.display = 'block';
                    updateDeviceControlButtons(device);
                    updateCommunicationLineButtons(device.id);
                    updateAllDevicePaths();
                } else {
                    // 역할이 명확하지 않은 경우 TX로 기본 처리
                    document.getElementById('obu-tx-sensor').style.display = 'block';
                    updateDeviceControlButtons(device);
                    updateCommunicationLineButtons(device.id);
                    updateAllDevicePaths();
                }
            } else if (device.type === 'RSU') {
                document.getElementById('rsu-sensor').style.display = 'block';
                updateCommunicationLineButtons(device.id);
            }
        }
        
        // 모든 센서 패널 숨기기 함수
        function hideSensorPanels() {
            document.getElementById('obu-tx-sensor').style.display = 'none';
            document.getElementById('obu-rx-sensor').style.display = 'none';
            document.getElementById('rsu-sensor').style.display = 'none';

            // TX 센서패널이 내려갈 때 CAN 패널도 같이 숨김
            const canDetailDiv = document.getElementById('obu-tx-can-detail');
            if (canDetailDiv) {
                canDetailDiv.style.display = 'none';
            }

            // 모든 제어 버튼 active 상태 초기화
            const controlButtons = document.querySelectorAll('.sensor-control-button');
            controlButtons.forEach(button => {
                button.classList.remove('active');
            });
        }
        
        // 선택된 장치 UI 업데이트 (하이라이트)
        function updateSelectedDeviceUI() {
            // 모든 장치에서 선택 표시 제거
            document.querySelectorAll('.device-item').forEach(el => {
                el.classList.remove('selected');
            });
            
            // 선택된 장치에 표시 추가
            if (selectedDevice) {
                const deviceId = `${selectedDevice.type}-${selectedDevice.id}`;
                const selectedElement = document.querySelector(`[data-device-id="${deviceId}"]`);
                if (selectedElement) {
                    selectedElement.classList.add('selected');
                }
            }
        }
        
        // 선택된 장치의 센서값으로 하단 테이블 업데이트
        function updateSensorValuesForSelectedDevice() {
            if (!selectedDevice) {
                return;
            }
            
            const device = selectedDevice;
            
            // 장치 상태 계산
            const lastSeenSeconds = Math.floor((Date.now() - device.lastSeen) / 1000);
            let timeStatusText;
            if (lastSeenSeconds < 1) {
                timeStatusText = '방금 전';
            } else {
                timeStatusText = `${lastSeenSeconds}초 전`;
            }
            
            // 장치 타입에 따라 다른 센서 패널 업데이트
            if (device.type === 'OBU') {
                if (device.role === 'Transmitter') {
                    updateObuTxSensorPanel(device, timeStatusText);
                } else if (device.role === 'Receiver') {
                    updateObuRxSensorPanel(device, timeStatusText);
            } else {
                    updateObuTxSensorPanel(device, timeStatusText); // 기본값
                }
            } else if (device.type === 'RSU') {
                updateRsuSensorPanel(device, timeStatusText);
            }
        }
        
        // OBU TX 센서 패널 업데이트
        function updateObuTxSensorPanel(device, timeStatusText) {
            document.getElementById('obu-tx-device-name').textContent = `OBU TX #${device.id}`;
            document.getElementById('obu-tx-status').textContent = device.isActive ? '연결됨' : '연결 끊김';
            document.getElementById('obu-tx-last-seen').textContent = timeStatusText;
            
            document.getElementById('obu-tx-device-id').textContent = device.id || '-';
            document.getElementById('obu-tx-latitude').textContent = 
                device.latitude !== undefined && device.latitude !== null ? device.latitude.toFixed(6) : '-';
            document.getElementById('obu-tx-longitude').textContent = 
                device.longitude !== undefined && device.longitude !== null ? device.longitude.toFixed(6) : '-';
            document.getElementById('obu-tx-speed').textContent = 
                device.speed !== undefined && device.speed !== null ? `${device.speed.toFixed(1)} km/h` : '-';
            document.getElementById('obu-tx-heading').textContent = 
                device.heading !== undefined && device.heading !== null ? `${device.heading.toFixed(1)}°` : '-';
            document.getElementById('obu-tx-sw-version').textContent = 
                `L1: ${device.swVerL1 || '-'} / L2: ${device.swVerL2 || '-'}`;
            document.getElementById('obu-tx-hw-version').textContent = 
                `L1: ${device.hwVerL1 || '-'} / L2: ${device.hwVerL2 || '-'}`;
            
            // 버튼 상태 업데이트
            const autoTrackBtn = document.getElementById('obu-tx-auto-track');
            const visiblePathBtn = document.getElementById('obu-tx-visible-path');
            
            autoTrackBtn.classList.toggle('active', device.isCentering || false);
            
            // KD Tree 사용 여부에 따라 다른 활성화 스타일 적용
            const useKdTree = window.deviceKdTreeUsage.get(String(device.id)) || false;
            if (device.isPathVisible) {
                if (useKdTree) {
                    visiblePathBtn.classList.remove('active');
                    visiblePathBtn.classList.add('active-kdtree');
                } else {
                    visiblePathBtn.classList.remove('active-kdtree');
                    visiblePathBtn.classList.add('active');
                }
            } else {
                visiblePathBtn.classList.remove('active', 'active-kdtree');
            }
            
            // AUTO TRACK이 활성화된 경우 지도 중심을 이 장치 위치로 이동하고 헤딩에 따라 회전
            if (device.isCentering && device.latitude && device.longitude && window.map) {
                const bearing = device.heading !== undefined && device.heading !== null ? reverseHeading(device.heading) : 0;
                window.map.easeTo({
                    center: [device.longitude, device.latitude],
                    bearing: bearing
                });
            }

            // OBU-TX 센서 패널 업데이트 함수 내부에 아래 코드 추가
            // CAN 값 확장/접기 토글 버튼 및 상세정보 영역 추가
            const sensorControls = document.querySelector('#obu-tx-sensor .sensor-controls');
            if (sensorControls && !document.getElementById('obu-tx-can-toggle-btn')) {
                const canToggleBtn = document.createElement('button');
                canToggleBtn.id = 'obu-tx-can-toggle-btn';
                canToggleBtn.className = 'sensor-control-button can-more-btn';
                canToggleBtn.textContent = 'CAN 값 더보기';
                canToggleBtn.style.cursor = 'pointer';
                sensorControls.appendChild(canToggleBtn);

                // 오른쪽 확장 패널 생성
                const canDetailDiv = document.createElement('div');
                canDetailDiv.id = 'obu-tx-can-detail';
                canDetailDiv.className = 'can-detail-panel';
                canDetailDiv.style.display = 'none';
                canDetailDiv.innerHTML = `
                  <div class="can-detail-header">
                    <span>CAN 상세정보</span>
                  </div>
                  <table class="can-detail-table">
                    <tr><th>조향각(Steer_Cmd)</th><td id="obu-tx-steer">-</td></tr>
                    <tr><th>가감속(Accel_Dec_Cmd)</th><td id="obu-tx-accel">-</td></tr>
                    <tr><th>EPS_En</th><td id="obu-tx-eps-en">-</td></tr>
                    <tr><th>Override_Ignore</th><td id="obu-tx-override">-</td></tr>
                    <tr><th>EPS_Speed</th><td id="obu-tx-eps-speed">-</td></tr>
                    <tr><th>ACC_En</th><td id="obu-tx-acc-en">-</td></tr>
                    <tr><th>AEB_En</th><td id="obu-tx-aeb-en">-</td></tr>
                    <tr><th>AEB_decel_value</th><td id="obu-tx-aeb-decel">-</td></tr>
                    <tr><th>Alive_Cnt</th><td id="obu-tx-alive">-</td></tr>
                    <tr><th>차속</th><td id="obu-tx-speed2">-</td></tr>
                    <tr><th>브레이크 압력</th><td id="obu-tx-brake">-</td></tr>
                    <tr><th>횡가속</th><td id="obu-tx-latacc">-</td></tr>
                    <tr><th>요레이트</th><td id="obu-tx-yawrate">-</td></tr>
                    <tr><th>조향각 센서</th><td id="obu-tx-steering-angle">-</td></tr>
                    <tr><th>조향 토크(운전자)</th><td id="obu-tx-steering-drv-tq">-</td></tr>
                    <tr><th>조향 토크(출력)</th><td id="obu-tx-steering-out-tq">-</td></tr>
                    <tr><th>EPS Alive Count</th><td id="obu-tx-eps-alive-cnt">-</td></tr>
                    <tr><th>ACC 상태</th><td id="obu-tx-acc-en-status">-</td></tr>
                    <tr><th>ACC 제어보드 상태</th><td id="obu-tx-acc-ctrl-bd-status">-</td></tr>
                    <tr><th>ACC 오류</th><td id="obu-tx-acc-err">-</td></tr>
                    <tr><th>ACC 사용자 CAN 오류</th><td id="obu-tx-acc-user-can-err">-</td></tr>
                    <tr><th>종가속</th><td id="obu-tx-long-accel">-</td></tr>
                    <tr><th>우회전 신호</th><td id="obu-tx-turn-right-en">-</td></tr>
                    <tr><th>위험신호</th><td id="obu-tx-hazard-en">-</td></tr>
                    <tr><th>좌회전 신호</th><td id="obu-tx-turn-left-en">-</td></tr>
                    <tr><th>ACC Alive Count</th><td id="obu-tx-acc-alive-cnt">-</td></tr>
                    <tr><th>가속페달 위치</th><td id="obu-tx-acc-pedal-pos">-</td></tr>
                    <tr><th>조향각 변화율</th><td id="obu-tx-steering-angle-rt">-</td></tr>
                    <tr><th>브레이크 작동 신호</th><td id="obu-tx-brake-act-signal">-</td></tr>
                  </table>
                `;
                // 센서패널 바로 뒤에 insert
                document.getElementById('obu-tx-sensor').after(canDetailDiv);

                canToggleBtn.onclick = function() {
                    const isOpen = canDetailDiv.style.display === 'flex';
                    if (!isOpen) {
                        canDetailDiv.style.display = 'flex';
                        canToggleBtn.classList.add('active');
                        setTimeout(syncCanPanelHeight, 100);
                    } else {
                        canDetailDiv.style.display = 'none';
                        canToggleBtn.classList.remove('active');
                    }
                };
            }
            // 센서 패널 업데이트 후에도 동기화 시도
            setTimeout(syncCanPanelHeight, 100);
            // 값 업데이트 (updateObuTxSensorPanel 내부에서 device 값으로)
            document.getElementById('obu-tx-steer').textContent = device.steer !== undefined ? `${device.steer.toFixed(2)}°` : '-';
            document.getElementById('obu-tx-accel').textContent = device.accel !== undefined ? `${device.accel.toFixed(2)} m/s²` : '-';
            document.getElementById('obu-tx-eps-en').textContent = device.epsEn !== undefined ? device.epsEn : '-';
            document.getElementById('obu-tx-override').textContent = device.overrideIgnore !== undefined ? device.overrideIgnore : '-';
            document.getElementById('obu-tx-eps-speed').textContent = device.epsSpeed !== undefined ? `${device.epsSpeed}` : '-';
            document.getElementById('obu-tx-acc-en').textContent = device.accEn !== undefined ? device.accEn : '-';
            document.getElementById('obu-tx-aeb-en').textContent = device.aebEn !== undefined ? device.aebEn : '-';
            document.getElementById('obu-tx-aeb-decel').textContent = device.aebDecel !== undefined ? `${device.aebDecel.toFixed(2)} G` : '-';
            document.getElementById('obu-tx-alive').textContent = device.aliveCnt !== undefined ? `${device.aliveCnt}` : '-';
            document.getElementById('obu-tx-speed2').textContent = device.speed2 !== undefined ? `${device.speed2} km/h` : '-';
            document.getElementById('obu-tx-brake').textContent = device.brake !== undefined ? `${device.brake.toFixed(2)} bar` : '-';
            document.getElementById('obu-tx-latacc').textContent = device.latacc !== undefined ? `${device.latacc.toFixed(2)} m/s²` : '-';
            document.getElementById('obu-tx-yawrate').textContent = device.yawrate !== undefined ? `${device.yawrate.toFixed(2)} °/s` : '-';
            document.getElementById('obu-tx-steering-angle').textContent = device.steeringAngle !== undefined ? `${device.steeringAngle.toFixed(2)}°` : '-';
            document.getElementById('obu-tx-steering-drv-tq').textContent = device.steeringDrvTq !== undefined ? `${device.steeringDrvTq.toFixed(2)} Nm` : '-';
            document.getElementById('obu-tx-steering-out-tq').textContent = device.steeringOutTq !== undefined ? `${device.steeringOutTq.toFixed(2)} Nm` : '-';
            document.getElementById('obu-tx-eps-alive-cnt').textContent = device.epsAliveCnt !== undefined ? `${device.epsAliveCnt}` : '-';
            document.getElementById('obu-tx-acc-en-status').textContent = device.accEnStatus !== undefined ? device.accEnStatus : '-';
            document.getElementById('obu-tx-acc-ctrl-bd-status').textContent = device.accCtrlBdStatus !== undefined ? `${device.accCtrlBdStatus}` : '-';
            document.getElementById('obu-tx-acc-err').textContent = device.accErr !== undefined ? `${device.accErr}` : '-';
            document.getElementById('obu-tx-acc-user-can-err').textContent = device.accUserCanErr !== undefined ? `${device.accUserCanErr}` : '-';
            document.getElementById('obu-tx-long-accel').textContent = device.longAccel !== undefined ? `${device.longAccel.toFixed(2)} m/s²` : '-';
            document.getElementById('obu-tx-turn-right-en').textContent = device.turnRightEn !== undefined ? device.turnRightEn : '-';
            document.getElementById('obu-tx-hazard-en').textContent = device.hazardEn !== undefined ? device.hazardEn : '-';
            document.getElementById('obu-tx-turn-left-en').textContent = device.turnLeftEn !== undefined ? device.turnLeftEn : '-';
            document.getElementById('obu-tx-acc-alive-cnt').textContent = device.accAliveCnt !== undefined ? `${device.accAliveCnt}` : '-';
            document.getElementById('obu-tx-acc-pedal-pos').textContent = device.accPedalPos !== undefined ? `${device.accPedalPos.toFixed(1)}%` : '-';
            document.getElementById('obu-tx-steering-angle-rt').textContent = device.steeringAngleRt !== undefined ? `${device.steeringAngleRt} °/s` : '-';
            document.getElementById('obu-tx-brake-act-signal').textContent = device.brakeActSignal !== undefined ? `${device.brakeActSignal}` : '-';
            // OBU-TX가 아닐 때는 버튼/상세정보 숨김
            if (device.role !== 'Transmitter') {
                if (document.getElementById('obu-tx-can-toggle-btn')) document.getElementById('obu-tx-can-toggle-btn').style.display = 'none';
                if (document.getElementById('obu-tx-can-detail')) document.getElementById('obu-tx-can-detail').style.display = 'none';
            } else {
                if (document.getElementById('obu-tx-can-toggle-btn')) document.getElementById('obu-tx-can-toggle-btn').style.display = 'block';
            }
        }
        
        // OBU RX 센서 패널 업데이트
        function updateObuRxSensorPanel(device, timeStatusText) {
            document.getElementById('obu-rx-device-name').textContent = `OBU RX #${device.id}`;
            document.getElementById('obu-rx-status').textContent = device.isActive ? '연결됨' : '연결 끊김';
            document.getElementById('obu-rx-last-seen').textContent = timeStatusText;
            
            document.getElementById('obu-rx-device-id').textContent = device.id || '-';
            document.getElementById('obu-rx-latitude').textContent = 
                device.latitude !== undefined && device.latitude !== null ? device.latitude.toFixed(6) : '-';
            document.getElementById('obu-rx-longitude').textContent = 
                device.longitude !== undefined && device.longitude !== null ? device.longitude.toFixed(6) : '-';
            document.getElementById('obu-rx-speed').textContent = 
                device.speed !== undefined && device.speed !== null ? `${device.speed.toFixed(1)} km/h` : '-';
            document.getElementById('obu-rx-heading').textContent = 
                device.heading !== undefined && device.heading !== null ? `${device.heading.toFixed(1)}°` : '-';
            document.getElementById('obu-rx-sw-version').textContent = 
                `L1: ${device.swVerL1 || '-'} / L2: ${device.swVerL2 || '-'}`;
            document.getElementById('obu-rx-hw-version').textContent = 
                `L1: ${device.hwVerL1 || '-'} / L2: ${device.hwVerL2 || '-'}`;
            document.getElementById('obu-rx-distance').textContent = 
                device.distance !== undefined && device.distance !== null ? `${device.distance.toFixed(2)} m` : '-';
            
            // 버튼 상태 업데이트
            const autoTrackBtn = document.getElementById('obu-rx-auto-track');
            const visiblePathBtn = document.getElementById('obu-rx-visible-path');
            
            autoTrackBtn.classList.toggle('active', device.isCentering || false);
            
            // KD Tree 사용 여부에 따라 다른 활성화 스타일 적용
            const useKdTree = window.deviceKdTreeUsage.get(String(device.id)) || false;
            if (device.isPathVisible) {
                if (useKdTree) {
                    visiblePathBtn.classList.remove('active');
                    visiblePathBtn.classList.add('active-kdtree');
                } else {
                    visiblePathBtn.classList.remove('active-kdtree');
                    visiblePathBtn.classList.add('active');
                }
            } else {
                visiblePathBtn.classList.remove('active', 'active-kdtree');
            }
            
            // AUTO TRACK이 활성화된 경우 지도 중심을 이 장치 위치로 이동하고 헤딩에 따라 회전
            if (device.isCentering && device.latitude && device.longitude && window.map) {
                const bearing = device.heading !== undefined && device.heading !== null ? reverseHeading(device.heading) : 0;
                window.map.easeTo({
                    center: [device.longitude, device.latitude],
                    bearing: bearing
                });
            }
        }
        
        // RSU 센서 패널 업데이트
        function updateRsuSensorPanel(device, timeStatusText) {
            document.getElementById('rsu-device-name').textContent = `RSU #${device.id}`;
            document.getElementById('rsu-status').textContent = '활성';
            document.getElementById('rsu-last-seen').textContent = '상시 연결';
            
            document.getElementById('rsu-device-id').textContent = device.id || '-';
            document.getElementById('rsu-latitude').textContent = 
                device.latitude !== undefined && device.latitude !== null ? device.latitude.toFixed(6) : '-';
            document.getElementById('rsu-longitude').textContent = 
                device.longitude !== undefined && device.longitude !== null ? device.longitude.toFixed(6) : '-';
            document.getElementById('rsu-role').textContent = device.role || 'Infrastructure';
            document.getElementById('rsu-coverage').textContent = '500m'; // 기본값
        }
        
        // 비활성 장치 체크 함수
        function checkInactiveDevices() {
            const now = Date.now();
            
            for (const [deviceId, device] of activeDevices) {
                const timeSinceLastSeen = now - device.lastSeen;
                if (timeSinceLastSeen > DEVICE_TIMEOUT) {
                    if (device.isActive) {
                        device.isActive = false;
                        // 장치 비활성화 로그 제거
                        
                        // 장치가 비활성화될 때 경로 데이터 초기화
                        device.isPathVisible = false;
                        if (typeof window.clearDevicePathData === 'function') {
                            window.clearDevicePathData(deviceId);
                        }
                    }
                    
                    // 30초 이상 비활성화된 장치는 완전 제거 (메모리 정리)
                    if (timeSinceLastSeen > DEVICE_TIMEOUT * 3) {
                        // 장치 완전 제거 로그 제거
                        // 장치 제거 전 경로 데이터 완전 초기화
                        if (typeof window.clearDevicePathData === 'function') {
                            window.clearDevicePathData(deviceId);
                        }
                        activeDevices.delete(deviceId);
                        window.deviceKdTreeUsage.delete(deviceId);
                    }
                } else if (!device.isActive) {
                    device.isActive = true;
                    // 장치 재활성화 로그 제거
                }
            }
        }
        
        // 주기적으로 장치 목록 업데이트 (100ms마다 - 실시간)
        setInterval(() => {
            updateDeviceListUI();
            updateSensorValuesForSelectedDevice(); // 실시간 센서값 업데이트
        }, 100);
        
        // 주기적으로 비활성 장치 체크 (1초마다)
        setInterval(() => {
            checkInactiveDevices();
        }, 1000);
        
        // 초기 RSU 장치 정보 추가 (판교 지역)
        function initializeRSUs() {
            const rsuList = [
                { id: '16', lat: 37.408940, lng: 127.099630 },
                { id: '17', lat: 37.406510, lng: 127.100833 },
                { id: '18', lat: 37.405160, lng: 127.103842 },
                { id: '5', lat: 37.410938, lng: 127.094749 },
                { id: '31', lat: 37.411751, lng: 127.095019 }
            ];
            
            rsuList.forEach(rsu => {
                updateDeviceInfo(`RSU-${rsu.id}`, 'RSU', {
                    latitude: rsu.lat,
                    longitude: rsu.lng,
                    role: 'Infrastructure'
                });
            });
        }
        
        // 초기 RSU 정보 설정
        setTimeout(() => {
            initializeRSUs();
        }, 2000);
        
        // 장치 리스트 초기 상태 설정 (접힌 상태)
        const obuList = document.getElementById('obu-list');
        const rsuList = document.getElementById('rsu-list');
        
        if (obuList) {
            obuList.style.display = 'none';
        }
        if (rsuList) {
            rsuList.style.display = 'none';
        }


        map.on('style.load', () => {
            // Fog 효과 제거
        });

        map.on('contextmenu', function (e) {
            const lngLat = e.lngLat;
            const latitude = lngLat.lat.toFixed(6);
            const longitude = lngLat.lng.toFixed(6);

            const popup = document.getElementById('coordinate-popup');
            const coordinateText = document.getElementById('coordinate-text');

            coordinateText.textContent = `위도(${latitude}) 경도(${longitude})`;

            popup.style.left = `${e.point.x}px`;
            popup.style.top = `${e.point.y}px`;

            popup.style.display = 'block';

            setTimeout(() => {
                popup.style.display = 'none';
            }, 5000);
        });

        map.on('style.load', function() {
            addRoadNetworkSource(); // 스타일 로드 후 즉시 소스 추가 시도
        });

        // --- 간소화된 JSON 웹소켓 데이터 처리 ---

        // JSON 파싱 함수 - 메모리 효율적 처리
        if (!window.jsonParseCache) {
            window.jsonParseCache = new Map();
        }
        const jsonParseCache = window.jsonParseCache;
        const MAX_CACHE_SIZE = 100;
        
        function parseJsonData(jsonString) {
            // 간단한 캠시 시스템 (동일한 JSON 문자열 반복 처리 최적화)
            if (jsonParseCache.has(jsonString)) {
                return jsonParseCache.get(jsonString);
            }
            
            try {
                const dataObj = JSON.parse(jsonString);
                
                // 캠시 크기 제한
                if (jsonParseCache.size >= MAX_CACHE_SIZE) {
                    const firstKey = jsonParseCache.keys().next().value;
                    jsonParseCache.delete(firstKey);
                }
                
                jsonParseCache.set(jsonString, dataObj);
                return dataObj;
            } catch (error) {
                console.error('[JSON] 파싱 오류:', error);
                return null;
            }
        }

        function handleWebSocketMessage(message) {
            // 개행문자로 분리 (각 라인이 완전한 JSON) - 메모리 효율적 처리
            const lines = message.data.split(/\r?\n/);
            
            // 배치 처리를 위한 배열 사전 할당
            const processedData = [];
            
            for (let line of lines) {
                line = line.trim();
                if (line === '') continue;
                
                // JSON 데이터 파싱
                const dataObj = parseJsonData(line);
                if (!dataObj) {
                    continue;
                }
                
                processedData.push(dataObj);
            }
            
            // 배치로 데이터 처리 (메모리 효율성 향상)
            for (const dataObj of processedData) {
                // 기존 데이터 처리 로직 유지
                if (isTxTest) {
                    s_unRxDevId = dataObj['unRxDeviceIdL1'] || dataObj['unRxDeviceIdL2'] || dataObj['unRxDeviceIdL3'];
                    s_nRxLatitude = parseFloat(dataObj['nRxLatitude']);
                    s_nRxLongitude = parseFloat(dataObj['nRxLongitude']);
                    s_unRxVehicleSpeed = parseFloat(dataObj['unRxVehicleSpeed']);
                    s_unRxVehicleHeading = parseFloat(dataObj['unRxVehicleHeading']);
                    s_nTxLatitude = vehicleLatitude1;
                    s_nTxLongitude = vehicleLongitude1;
                    s_unTxVehicleHeading = 90;
                    s_unPdr = 100;
                    s_ulLatencyL1 = 500;
                    s_ulTotalPacketCnt = 1 + (s_unTempTxCnt || 0);
                    s_unSeqNum = 1 + (s_unTempTxCnt || 0);
                    s_unTempTxCnt = (s_unTempTxCnt || 0) + 1;
                } else {
                    s_unRxDevId = dataObj['unRxDeviceIdL2'] || dataObj['unRxDeviceIdL3'] || dataObj['unRxDeviceIdL1'] || dataObj['unRxTargetDeviceId'];
                    
                    s_nRxLatitude = parseFloat(dataObj['nRxLatitude']) || convertCoordinate(dataObj['nLvLatitude']);
                    s_nRxLongitude = parseFloat(dataObj['nRxLongitude']) || convertCoordinate(dataObj['nLvLongitude']);
                    s_nRxAttitude = parseFloat(dataObj['nRxAttitude']);
                    s_unRxVehicleSpeed = parseFloat(dataObj['unRxVehicleSpeed']) || parseFloat(dataObj['usLvSpeed']);
                    s_unRxVehicleHeading = parseFloat(dataObj['unRxVehicleHeading']) || parseFloat(dataObj['usLvHeading']);
                    s_unTxDevId = dataObj['unTxDeviceIdL2'] || dataObj['unTxDeviceIdL3'] || dataObj['unTxDeviceIdL1'] || dataObj['unDeviceId'];
                    s_nTxLatitude = parseFloat(dataObj['nTxLatitude']);
                    s_nTxLongitude = parseFloat(dataObj['nTxLongitude']);
                    s_nTxAttitude = parseFloat(dataObj['nTxAttitude']);
                    s_unTxVehicleSpeed = parseFloat(dataObj['unTxVehicleSpeed']);
                    s_unTxVehicleHeading = parseFloat(dataObj['unTxVehicleHeading']);
                    s_unPdr = parseFloat(dataObj['unPdr(percent)']) || parseFloat(dataObj['unPer(percent)']);
                    s_ulLatencyL1 = parseFloat(dataObj['ulLatencyL1(us)']);
                    s_ulTotalPacketCnt = parseFloat(dataObj['ulTotalPacketCnt']);
                    s_unSeqNum = parseFloat(dataObj['unSeqNum']);
                    s_usCommDistance = parseFloat(dataObj['usCommDistance']);
                    s_nRssi = parseFloat(dataObj['nRssi']);
                    s_ucRcpi = parseFloat(dataObj['ucRcpi']);
                    s_usTxSwVerL1 = dataObj['usTxSwVerL1'];
                    s_usTxSwVerL2 = dataObj['usTxSwVerL2'];
                    s_usRxSwVerL1 = dataObj['usRxSwVerL1'];
                    s_usRxSwVerL2 = dataObj['usRxSwVerL2'];
                    s_usTxHwVerL1 = dataObj['usTxHwVerL1'];
                    s_usTxHwVerL2 = dataObj['usTxHwVerL2'];
                    s_usRxHwVerL1 = dataObj['usRxHwVerL1'];
                    s_usRxHwVerL2 = dataObj['usRxHwVerL2'];
                    
                    // CAN 값들 파싱 (JSON에서 직접 가져오기)
                    
                    // CAN 관련 컬럼 매핑 정의
                    const canColumnMapping = {
                        steer: ['fSteeringCmd', 'SteeringCmd', 'steering_cmd'],
                        accel: ['fAccelCmd', 'AccelCmd', 'accel_cmd'],
                        epsEn: ['bEpsEnable', 'EpsEnable', 'eps_enable'],
                        overrideIgnore: ['bOverrideIgnore', 'OverrideIgnore', 'override_ignore'],
                        epsSpeed: ['ucEpsSpeed', 'EpsSpeed', 'eps_speed'],
                        accEn: ['bAccEnable', 'AccEnable', 'acc_enable'],
                        aebEn: ['bAebEnable', 'AebEnable', 'aeb_enable'],
                        aebDecel: ['fAebDecelValue', 'AebDecelValue', 'aeb_decel_value'],
                        aliveCnt: ['ucAliveCnt', 'AliveCnt', 'alive_cnt'],
                        speed2: ['ucVehicleSpeed', 'VehicleSpeed', 'vehicle_speed'],
                        brake: ['fBrakeCylinder', 'BrakeCylinder', 'brake_cylinder'],
                        latacc: ['fLatAccel', 'LatAccel', 'lat_accel'],
                        yawrate: ['fYawRate', 'YawRate', 'yaw_rate'],
                        steeringAngle: ['fSteeringAngle', 'SteeringAngle', 'steering_angle'],
                        steeringDrvTq: ['fSteeringDrvTq', 'SteeringDrvTq', 'steering_drv_tq'],
                        steeringOutTq: ['fSteeringOutTq', 'SteeringOutTq', 'steering_out_tq'],
                        epsAliveCnt: ['ucEpsAliveCnt', 'EpsAliveCnt', 'eps_alive_cnt'],
                        accEnStatus: ['bAccEnStatus', 'AccEnStatus', 'acc_en_status'],
                        accCtrlBdStatus: ['ucAccCtrlBdStatus', 'AccCtrlBdStatus', 'acc_ctrl_bd_status'],
                        accErr: ['ucAcAccErr', 'AcAccErr', 'ac_acc_err'],
                        accUserCanErr: ['ucAccUserCanErr', 'AccUserCanErr', 'acc_user_can_err'],
                        longAccel: ['fLongAccel', 'LongAccel', 'long_accel'],
                        turnRightEn: ['bTurnRightEn', 'TurnRightEn', 'turn_right_en'],
                        hazardEn: ['bHazardEn', 'HazardEn', 'hazard_en'],
                        turnLeftEn: ['bTurnLeftEn', 'TurnLeftEn', 'turn_left_en'],
                        accAliveCnt: ['ucAccAliveCnt', 'AccAliveCnt', 'acc_alive_cnt'],
                        accPedalPos: ['fAccPedalPos', 'AccPedalPos', 'acc_pedal_pos'],
                        steeringAngleRt: ['unSteeringAngleRt', 'SteeringAngleRt', 'steering_angle_rt'],
                        brakeActSignal: ['ucBrakeActSignal', 'BrakeActSignal', 'brake_act_signal']
                    };
                    
                    // JSON에서 직접 CAN 값 찾기
                    function findJsonValue(possibleNames) {
                        for (const name of possibleNames) {
                            if (dataObj.hasOwnProperty(name)) {
                                return dataObj[name];
                            }
                        }
                        return null;
                    }
                    
                    // CAN 값들 파싱 - JSON 기반
                    const canValues = {};
                    
                    for (const [key, possibleNames] of Object.entries(canColumnMapping)) {
                        const value = findJsonValue(possibleNames);
                        
                        if (value !== null) {
                            switch (key) {
                                case 'epsEn':
                                case 'accEn':
                                case 'aebEn':
                                case 'accEnStatus':
                                    canValues[key] = value === '1' ? 'Enabled' : 'Disabled';
                                    break;
                                case 'overrideIgnore':
                                    canValues[key] = value === '1' ? 'Yes' : 'No';
                                    break;
                                case 'turnRightEn':
                                case 'hazardEn':
                                case 'turnLeftEn':
                                    canValues[key] = value === '1' ? 'On' : 'Off';
                                    break;
                                default:
                                    canValues[key] = parseFloat(value) || 0;
                            }
                        } else {
                            // 컬럼을 찾지 못한 경우 기본값 설정
                            switch (key) {
                                case 'epsEn':
                                case 'accEn':
                                case 'aebEn':
                                case 'accEnStatus':
                                    canValues[key] = 'Disabled';
                                    break;
                                case 'overrideIgnore':
                                    canValues[key] = 'No';
                                    break;
                                case 'turnRightEn':
                                case 'hazardEn':
                                case 'turnLeftEn':
                                    canValues[key] = 'Off';
                                    break;
                                default:
                                    canValues[key] = 0;
                            }
                        }
                    }
                    
                    // 전역 변수로 저장
                    window.lastCanValues = canValues;
                }
                // 주요 값 로그 (디버깅용)
                // //console.log('s_unPdr:', s_unPdr, 's_ulTotalPacketCnt:', s_ulTotalPacketCnt);
                fetchAndUpdate();
            }
        }

        if ('WebSocket' in window) {
            let ws = new WebSocket(`ws://${ipAddress}:3001/websocket`);
            ws.onopen = () => {
                console.log('[WebSocket] 연결 성공');
                ws.send('Client connected');
            };
            function reverseHeading(heading) {
                let reversedHeading = (360 - ((parseInt(heading) + 180) % 360)) % 360;
                return reversedHeading;
            }
            ws.onmessage = handleWebSocketMessage;
            ws.onerror = (error) => {
                console.error('[WebSocket] 연결 오류:', error);
            };
            ws.onclose = (event) => {
                console.log('[WebSocket] 연결 종료:', event.code, event.reason);
            };
        } else {
            console.error('[WebSocket] 브라우저에서 WebSocket을 지원하지 않습니다');
        }



        /************************************************************/
        /* KD Tree */
        /************************************************************/
        let roadNetworkCoordinates = [];
        let tree;

        map.on('style.load', () => {
            //console.log("Map style loaded successfully.");
            addRoadNetworkSource(); // 스타일 로드 후 즉시 소스 추가 시도
        });

        function addRoadNetworkSource() {
            fetch('https://raw.githubusercontent.com/KETI-A/athena/main/src/packages/maps/ctrack-utm52n_ellipsoid/c-track-a2-link.geojson')
                .then(response => {
                    if (!response.ok) {
                        throw new Error("Network response was not ok");
                    }
                    return response.json(); // JSON으로 변환
                })
                .then(geojsonData => {
                    // GeoJSON 데이터가 유효한지 확인
                    if (geojsonData && geojsonData.type === 'FeatureCollection' && Array.isArray(geojsonData.features)) {
                        // 소스가 이미 존재하는지 확인
                        if (!map.getSource('road-network')) {
                        // GeoJSON 데이터를 Mapbox에 소스로 추가
                        map.addSource('road-network', {
                            'type': 'geojson',
                            'data': geojsonData
                        });
                        }

                        // KD-Tree 빌드 함수 호출
                        //buildKdTreeFromGeoJSON(geojsonData);
                        buildKdTreeInterPolateFromGeoJSON(geojsonData);
                    } else {
                        console.error("GeoJSON data is invalid or missing features.");
                    }
                })
                .catch(error => {
                    console.error("Failed to fetch GeoJSON data:", error);
                });
        }

        class KDTree {
            constructor(points, metric) {
                this.metric = metric;
                this.dimensions = [0, 1];  // 경도와 위도 인덱스 (0: longitude, 1: latitude)
                this.root = this.buildTree(points, 0);
            }

            buildTree(points, depth) {
                if (points.length === 0) return null;

                const axis = depth % this.dimensions.length; // 경도와 위도를 번갈아가며 분할
                points.sort((a, b) => a[axis] - b[axis]); // 각 축에 따라 정렬

                const median = Math.floor(points.length / 2); // 중간값

                return {
                    point: points[median], // 중간값 기준으로 노드를 설정
                    left: this.buildTree(points.slice(0, median), depth + 1), // 왼쪽 하위 트리
                    right: this.buildTree(points.slice(median + 1), depth + 1) // 오른쪽 하위 트리
                };
            }

            nearest(point, maxNodes = 1) {
                const bestNodes = new BinaryHeap((e) => -e[1]);  // 최소 힙을 사용하여 최근접 노드를 추적

                const nearestSearch = (node, depth) => {
                    if (node === null) return;

                    const axis = depth % this.dimensions.length; // 현재 비교할 축 (경도 또는 위도)

                    const ownDistance = this.metric(point, node.point); // 현재 노드와의 거리 계산
                    const linearPoint = [...point]; // 입력된 좌표의 복사본을 만듦
                    linearPoint[axis] = node.point[axis]; // 한 축을 고정하여 계산

                    let bestChild = null;
                    let otherChild = null;

                    // 입력된 좌표와 현재 노드의 비교 축에 따라 왼쪽 또는 오른쪽 자식 노드 선택
                    if (point[axis] < node.point[axis]) {
                        bestChild = node.left;
                        otherChild = node.right;
                    } else {
                        bestChild = node.right;
                        otherChild = node.left;
                    }

                    // 더 가까운 쪽 자식 노드를 먼저 검색
                    nearestSearch(bestChild, depth + 1);

                    // 현재 노드와의 거리가 현재 가장 가까운 거리보다 작다면 갱신
                    if (bestNodes.size() < maxNodes || ownDistance < bestNodes.peek()[1]) {
                        bestNodes.push([node.point, ownDistance]);
                        if (bestNodes.size() > maxNodes) bestNodes.pop();
                    }

                    const linearDistance = this.metric(linearPoint, node.point); // 축을 고정한 거리 계산

                    // 고정된 축을 기준으로 계산된 거리가 더 가까울 수 있는지 검사
                    if (bestNodes.size() < maxNodes || linearDistance < bestNodes.peek()[1]) {
                        nearestSearch(otherChild, depth + 1);
                    }
                };

                nearestSearch(this.root, 0);

                const result = [];
                while (bestNodes.size()) {
                    result.push(bestNodes.pop()[0]);
                }

                return result;
            }
        }

        class BinaryHeap {
            constructor(scoreFunction) {
                this.content = [];
                this.scoreFunction = scoreFunction;
            }

            push(element) {
                this.content.push(element);
                this.bubbleUp(this.content.length - 1);
            }

            pop() {
                const result = this.content[0];
                const end = this.content.pop();
                if (this.content.length > 0) {
                    this.content[0] = end;
                    this.sinkDown(0);
                }
                return result;
            }

            size() {
                return this.content.length;
            }

            peek() {
                return this.content[0];
            }

            bubbleUp(n) {
                const element = this.content[n];
                const score = this.scoreFunction(element);

                while (n > 0) {
                    const parentN = Math.floor((n + 1) / 2) - 1;
                    const parent = this.content[parentN];
                    if (score >= this.scoreFunction(parent)) break;
                    this.content[parentN] = element;
                    this.content[n] = parent;
                    n = parentN;
                }
            }

            sinkDown(n) {
                const length = this.content.length;
                const element = this.content[n];
                const elemScore = this.scoreFunction(element);

                while (true) {
                    const child2N = (n + 1) * 2;
                    const child1N = child2N - 1;
                    let swap = null;
                    let child1Score;

                    if (child1N < length) {
                        const child1 = this.content[child1N];
                        child1Score = this.scoreFunction(child1);
                        if (child1Score < elemScore) swap = child1N;
                    }

                    if (child2N < length) {
                        const child2 = this.content[child2N];
                        const child2Score = this.scoreFunction(child2);
                        if (child2Score < (swap === null ? elemScore : child1Score)) swap = child2N;
                    }

                    if (swap === null) break;

                    this.content[n] = this.content[swap];
                    this.content[swap] = element;
                    n = swap;
                }
            }
        }

        // 유클리드 거리 측정 함수
        function euclideanDistance(a, b) {
            const dx = a[0] - b[0]; // 경도 차이
            const dy = a[1] - b[1]; // 위도 차이
            return Math.sqrt(dx * dx + dy * dy);
        }

        function buildKdTreeFromGeoJSON(geojsonData) {
            let roadNetworkCoordinates = [];

            // MultiLineString 및 LineString 처리
            geojsonData.features.forEach(feature => {
                if (feature.geometry.type === "LineString") {
                    feature.geometry.coordinates.forEach(coord => {
                        roadNetworkCoordinates.push([coord[0], coord[1]]);
                    });
                } else if (feature.geometry.type === "MultiLineString") {
                    feature.geometry.coordinates.forEach(line => {
                        line.forEach(coord => {
                            //console.log("KD-Tree Input Coordinate:", coord); // 좌표를 출력
                            roadNetworkCoordinates.push([coord[0], coord[1]]);
                        });
                    });
                }
            });

            // KD-Tree 생성 및 전역 변수로 설정
            tree = new KDTree(roadNetworkCoordinates, euclideanDistance);

            //console.log("KD-Tree built successfully with", roadNetworkCoordinates.length, "points.");

            // KD-Tree 좌표를 지도에 추가
            if(isVisiblePath) {
                addPointsToMapOfKdTree(roadNetworkCoordinates);
            }
        }

        function buildKdTreeInterPolateFromGeoJSON(geojsonData) {
            let roadNetworkCoordinates = [];

            // MultiLineString 및 LineString 처리
            geojsonData.features.forEach(feature => {
                if (feature.geometry.type === "LineString") {
                    for (let i = 0; i < feature.geometry.coordinates.length - 1; i++) {
                        let startCoord = feature.geometry.coordinates[i];
                        let endCoord = feature.geometry.coordinates[i + 1];

                        // 두 점 사이의 거리 계산 (Haversine Formula 사용)
                        let distance = haversineDistance([startCoord[0], startCoord[1]], [endCoord[0], endCoord[1]]);
                        //console.log(`Distance between points: ${distance} meters`);

                        // 원래 좌표 추가
                        roadNetworkCoordinates.push([startCoord[0], startCoord[1]]);

                        // 두 점 사이의 거리가 1m 이상 10m 이하일 때만 보간
                        if (distance >= 1 && distance <= 10) {
                            // 두 점 사이를 30cm 단위로 보간하여 추가
                            let interpolatedPoints = interpolatePoints([startCoord, endCoord], 0.3);  // 간격을 30cm로 설정
                            roadNetworkCoordinates.push(...interpolatedPoints);  // 보간된 좌표 추가
                        } else {
                            //console.log(`No interpolation: Distance is ${distance} meters`);
                        }

                        // 마지막 점 추가
                        if (i === feature.geometry.coordinates.length - 2) {
                            roadNetworkCoordinates.push([endCoord[0], endCoord[1]]);
                        }
                    }
                } else if (feature.geometry.type === "MultiLineString") {
                    feature.geometry.coordinates.forEach(line => {
                        for (let i = 0; i < line.length - 1; i++) {
                            let startCoord = line[i];
                            let endCoord = line[i + 1];

                            // 두 점 사이의 거리 계산 (Haversine Formula 사용)
                            let distance = haversineDistance([startCoord[0], startCoord[1]], [endCoord[0], endCoord[1]]);
                            //console.log(`Distance between points: ${distance} meters`);

                            // 원래 좌표 추가
                            roadNetworkCoordinates.push([startCoord[0], startCoord[1]]);

                            // 두 점 사이의 거리가 1m 이상 10m 이하일 때만 보간
                            if (distance >= 1 && distance <= 10) {
                                // 두 점 사이를 30cm 단위로 보간하여 추가
                                let interpolatedPoints = interpolatePoints([startCoord, endCoord], 0.3);  // 간격을 30cm로 설정
                                roadNetworkCoordinates.push(...interpolatedPoints);  // 보간된 좌표 추가
                            } else {
                               //console.log(`No interpolation: Distance is ${distance} meters`);
                            }

                            // 마지막 점 추가
                            if (i === line.length - 2) {
                                roadNetworkCoordinates.push([endCoord[0], endCoord[1]]);
                            }
                        }
                    });
                }
            });

            // KD-Tree 생성 및 전역 변수로 설정
            //tree = new KDTree(roadNetworkCoordinates, euclideanDistance);
            // KD-Tree 생성 시 거리 계산을 Haversine Formula로 변경
            tree = new KDTree(roadNetworkCoordinates, haversineDistance);

            //console.log("KD-Tree built successfully with", roadNetworkCoordinates.length, "points.");


        }

        // Haversine formula를 사용한 거리 계산 (메트릭 단위로 반환)
        function haversineDistance(coord1, coord2) {
            const R = 6371000; // 지구의 반지름 (미터)
            const lat1 = toRadians(coord1[1]);
            const lat2 = toRadians(coord2[1]);
            const deltaLat = toRadians(coord2[1] - coord1[1]);
            const deltaLon = toRadians(coord2[0] - coord1[0]);

            const a = Math.sin(deltaLat / 2) * Math.sin(deltaLat / 2) +
                      Math.cos(lat1) * Math.cos(lat2) *
                      Math.sin(deltaLon / 2) * Math.sin(deltaLon / 2);

            const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

            return R * c; // 거리 (미터 단위)
        }

        // 각도를 라디안으로 변환
        function toRadians(degrees) {
            return degrees * Math.PI / 180;
        }
        
        // 전역 함수로 노출
        window.haversineDistance = haversineDistance;
        window.toRadians = toRadians;

        // 보간 함수: 두 점 사이의 빈 공간을 30cm 단위로 채우는 좌표 생성
        function interpolatePoints(points, spacing) {
            let interpolatedPoints = [];
            let [x1, y1] = points[0];
            let [x2, y2] = points[1];
            let distance = haversineDistance([x1, y1], [x2, y2]);
            let steps = Math.floor(distance / spacing);

            for (let j = 1; j < steps; j++) {  // 30cm 간격으로 중간 점 추가 (끝 점 제외)
                let t = j / steps;
                let x = x1 + t * (x2 - x1);
                let y = y1 + t * (y2 - y1);
                interpolatedPoints.push([x, y]);
            }

            return interpolatedPoints;
        }

        // 지도에 점 추가 함수
        function addPointsToMapOfKdTree(coordinates) {
            // 지도에 추가할 점 데이터
            const points = coordinates.map(coord => ({
                'type': 'Feature',
                'geometry': {
                    'type': 'Point',
                    'coordinates': coord
                }
            }));

            // 기존의 소스가 있으면 삭제
            if (map.getSource('kd-tree-points')) {
                map.removeSource('kd-tree-points');
            }

            // 지도에 점 추가
            map.addSource('kd-tree-points', {
                'type': 'geojson',
                'data': {
                    'type': 'FeatureCollection',
                    'features': points
                }
            });

            // 기존의 레이어가 있으면 삭제
            if (map.getLayer('kd-tree-points-layer')) {
                map.removeLayer('kd-tree-points-layer');
            }

            map.addLayer({
                'id': 'kd-tree-points-layer',
                'type': 'circle',
                'source': 'kd-tree-points',
                'paint': {
                    'circle-radius': 3,
                    'circle-color': '#00FF00'
                }
            });
            //console.log("Added points to map:", points);
        }

        /************************************************************/
        /* Map */
        /************************************************************/
        map.on('load', () => {
            //console.log("Map loaded successfully.");

            map.on('zoom', () => {
                map.resize();  // 확대/축소 시 강제로 지도를 리렌더링
            });

            map.on('move', () => {
                map.resize();  // 지도가 이동될 때 리렌더링
            });

            /************************************************************/
            /* CI - COMMENTED OUT */
            /************************************************************/
            /*
            map.loadImage(
                'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/keti_ci.png',
                (error, image) => {
                    if (error) throw error;

                    map.addImage('keti-log', image);

                    map.addSource('keti_log_src', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': [
                                {
                                    'type': 'Feature',
                                    'geometry': {
                                        'type': 'Point',
                                        'coordinates': [cKetiPangyolongitude, cKetiPangyoLatitude]
                                    }
                                }
                            ]
                        }
                    });

                    map.addLayer({
                        'id': 'keti',
                        'type': 'symbol',
                        'source': 'keti_log_src',
                        'layout': {
                            'icon-image': 'keti-log',
                            'icon-size': 0.50,
                            'icon-allow-overlap': true
                        }
                    });
                }
            );
            */

            /************************************************************/
            /* RSU - COMMENTED OUT */
            /************************************************************/
            /*
            map.loadImage(
                'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/fixed-rsu.png',
                (error, image) => {
                    if (error) throw error;

                    map.addImage('rsu', image);

                    map.addSource('rsu-src-18', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': [
                                {
                                    'type': 'Feature',
                                    'geometry': {
                                        'type': 'Point',
                                        'coordinates': [cPangyoRsuLongitude18, cPangyoRsuLatitude18]
                                    }
                                }
                            ]
                        }
                    });

                    map.addLayer({
                        'id': 'rsu18',
                        'type': 'symbol',
                        'source': 'rsu-src-18',
                        'layout': {
                            'icon-image': 'rsu',
                            'icon-size': 0.4,
                            'icon-allow-overlap': true
                        }
                    });

                    map.addSource('rsu-src-16', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': [
                                {
                                    'type': 'Feature',
                                    'geometry': {
                                        'type': 'Point',
                                        'coordinates': [cPangyoRsuLongitude16, cPangyoRsuLatitude16]
                                    }
                                }
                            ]
                        }
                    });

                    map.addLayer({
                        'id': 'rsu16',
                        'type': 'symbol',
                        'source': 'rsu-src-16',
                        'layout': {
                            'icon-image': 'rsu',
                            'icon-size': 0.4,
                            'icon-allow-overlap': true
                        }
                    });

                    map.addSource('rsu-src-17', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': [
                                {
                                    'type': 'Feature',
                                    'geometry': {
                                        'type': 'Point',
                                        'coordinates': [cPangyoRsuLongitude17, cPangyoRsuLatitude17]
                                    }
                                }
                            ]
                        }
                    });

                    map.addLayer({
                        'id': 'rsu17',
                        'type': 'symbol',
                        'source': 'rsu-src-17',
                        'layout': {
                            'icon-image': 'rsu',
                            'icon-size': 0.4,
                            'icon-allow-overlap': true
                        }
                    });

                }
            );

            map.addSource('rsu-src-31', {
                'type': 'geojson',
                'data': {
                    'type': 'FeatureCollection',
                    'features': [
                        {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': [cPangyoRsuLongitude31, cPangyoRsuLatitude31]
                            }
                        }
                    ]
                }
            });

            map.addLayer({
                'id': 'rsu31',
                'type': 'symbol',
                'source': 'rsu-src-31',
                'layout': {
                    'icon-image': 'rsu',
                    'icon-size': 0.4,
                    'icon-allow-overlap': true
                }
            });

            map.addSource('rsu-src-5', {
                'type': 'geojson',
                'data': {
                    'type': 'FeatureCollection',
                    'features': [
                        {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': [cPangyoRsuLongitude5, cPangyoRsuLatitude5]
                            }
                        }
                    ]
                }
            });

            map.addLayer({
                'id': 'rsu5',
                'type': 'symbol',
                'source': 'rsu-src-5',
                'layout': {
                    'icon-image': 'rsu',
                    'icon-size': 0.4,
                    'icon-allow-overlap': true
                }
            });
            */

            /************************************************************/
            /* Vehicle 0 */
            /************************************************************/
            map.loadImage(vehicle0ImageUrl, (error, image) => {
                    if (error) throw error;

                    map.addImage('vehicle', image);

                    map.addSource('vehicle_src_0', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': [
                                {
                                    'type': 'Feature',
                                    'geometry': {
                                        'type': 'Point',
                                        'coordinates': [vehicleLongitude0, vehicleLatitude0]
                                    }
                                }
                            ]
                        }
                    });

                    map.addLayer({
                        'id': 'vehicle0',
                        'type': 'symbol',
                        'source': 'vehicle_src_0',
                        'layout': {
                            'icon-image': 'vehicle',
                            'icon-size': [
                                'interpolate',
                                ['exponential', 2],
                                ['zoom'],
                                1, 0.001,
                                5, 0.008,
                                10, 0.04,
                                15, 0.126,
                                20, 0.168
                            ],
                            'icon-rotate': ['get', 'heading'],
                            'icon-rotation-alignment': 'map',
                            'icon-pitch-alignment': 'viewport',
                            'text-field': [
                                'concat',
                                ['case',
                                    ['==', ['get', 'deviceID'], CVehId], 'C-VEH#',
                                    ['==', ['get', 'deviceID'], AVehId], 'A-VEH#',
                                    'OBU#'
                                ],
                                ['get', 'deviceID']
                            ],
                            'text-font': ['Open Sans Semibold', 'Arial Unicode MS Bold'],
                            'text-offset': [0, 2], // Adjust this value to position the text
                            'text-anchor': 'top',
                            'text-size': [
                                'interpolate',
                                ['exponential', 2],
                                ['zoom'],
                                1, 2,     // 줌 아웃할 때 작게
                                5, 4,
                                10, 6,
                                15, 13,   // 기본값
                                20, 13    // 줌 인할 때 크게
                            ],
                            'icon-allow-overlap': true,
                            'text-allow-overlap': true
                        },
                        'paint': {
                            'text-color': '#000000',
                            'text-halo-color': '#FFFFFF',
                            'text-halo-width': 2
                        }
                    });

                    map.moveLayer('vehicle0');

                    fetchAndUpdate();
                    setInterval(fetchAndUpdate, 100);
                }
            );

            /************************************************************/
            /* Vehicle 1 */
            /************************************************************/
            map.loadImage(vehicle1ImageUrl, (error, image) => {
                    if (error) throw error;

                    map.addImage('vehicle1', image);

                    map.addSource('vehicle_src_1', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': [
                                {
                                    'type': 'Feature',
                                    'geometry': {
                                        'type': 'Point',
                                        'coordinates': [vehicleLongitude1, vehicleLatitude1]
                                    }
                                }
                            ]
                        }
                    });

                    map.addLayer({
                        'id': 'vehicle1',
                        'type': 'symbol',
                        'source': 'vehicle_src_1',
                        'layout': {
                            'icon-image': 'vehicle1',
                            'icon-size': [
                                'interpolate',
                                ['exponential', 2],
                                ['zoom'],
                                1, 0.001,
                                5, 0.008,
                                10, 0.04,
                                15, 0.126,
                                20, 0.168
                            ],
                            'icon-rotate': ['get', 'heading'],
                            'icon-rotation-alignment': 'map',
                            'icon-pitch-alignment': 'viewport',
                            'text-field': [
                                'concat',
                                ['case',
                                    ['==', ['get', 'deviceID'], CVehId], 'C-VEH#',
                                    ['==', ['get', 'deviceID'], AVehId], 'A-VEH#',
                                    'OBU#'
                                ],
                                ['get', 'deviceID']
                            ],
                            'text-font': ['Open Sans Semibold', 'Arial Unicode MS Bold'],
                            'text-offset': [0, 2],
                            'text-anchor': 'top',
                            'text-size': [
                                'interpolate',
                                ['exponential', 2],
                                ['zoom'],
                                1, 2,     // 줌 아웃할 때 작게
                                5, 4,
                                10, 6,
                                15, 13,   // 기본값
                                20, 13    // 줌 인할 때 크게
                            ],
                            'icon-allow-overlap': true,
                            'text-allow-overlap': true
                        },
                        'paint': {
                            'text-color': '#000000',
                            'text-halo-color': '#FFFFFF',
                            'text-halo-width': 2
                        }
                    });

                    map.moveLayer('vehicle1');

                    fetchAndUpdate();
                    setInterval(fetchAndUpdate, 100);
                }
            );

            //console.log("Road network and vehicle sources added successfully.");
        });

        /************************************************************/
        /* Update Position */
        /************************************************************/
        // 이전 좌표를 저장할 객체 (vehicleId에 따라 다르게 저장)
        let previousCoordinatesMap = {};

        function updateVehiclePosition(vehicleId, coordinates, heading, deviceId) {
            let vehicleSource = map.getSource(`vehicle_src_${vehicleId}`);
            let snappedCoordinates = coordinates;  // 기본값을 실제 GPS 좌표로 설정
            let maxAllowedShift = 5;  // 허용 가능한 최대 이동 거리 (미터 단위)
            
            // 차량 소스가 아직 생성되지 않았으면 업데이트 건너뛰기
            if (!vehicleSource) {
                return; // 소스가 없으면 조용히 리턴 (경고 로그 제거)
            }
            
            // 현재 장치 정보 가져오기
            const currentDevice = activeDevices.get(String(deviceId));
            
            // 디버그 로그 제거

            // 이전 좌표가 존재하지 않으면 현재 좌표를 이전 좌표로 설정
            if (!previousCoordinatesMap[vehicleId]) {
                previousCoordinatesMap[vehicleId] = coordinates; // 이전 좌표를 현재 좌표로 초기화
            }

            let previousCoordinates = previousCoordinatesMap[vehicleId];  // 현재 차량의 이전 좌표

            /* KD Tree Path: 장치별 KD Tree 사용 여부에 따라 스냅된 좌표로 업데이트 */
            const deviceIdStr = String(deviceId);
            const useKdTree = window.deviceKdTreeUsage.get(deviceIdStr) || false;
            
            // KD Tree 사용 여부 로그 제거
            
            
            if (tree && useKdTree) {
                let point = {
                    longitude: coordinates[0],
                    latitude: coordinates[1]
                };

                let nearest = tree.nearest([point.longitude, point.latitude], 1);

                if (nearest.length > 0) {
                    let nearestPoint = nearest[0];
                    let distanceThreshold = 2; // 허용 가능한 스냅 거리 (미터 단위)

                    // Haversine 공식을 사용하여 현재 좌표와 KD 트리에서 선택된 좌표 사이의 거리 계산
                    let distance = haversineDistance([point.longitude, point.latitude], nearestPoint);

                    // 필터링: 거리가 임계값 이하일 경우에만 스냅된 좌표로 업데이트
                    if (distance < distanceThreshold) {
                        //console.log(`Distance: ${distance} meters, Distance Threshold: ${distanceThreshold} meters`);

                        // Haversine 공식을 사용하여 이전 좌표와 KD 트리에서 선택된 좌표 간의 이동 거리 계산
                        let shift = haversineDistance(previousCoordinates, nearestPoint);

                        if (shift < maxAllowedShift) {
                            snappedCoordinates = nearestPoint; // 조건을 충족하는 경우에만 스냅된 좌표 사용
                        }
                    }

                    vehicleSource.setData({
                        'type': 'FeatureCollection',
                        'features': [{
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': snappedCoordinates
                            },
                            'properties': {
                                'heading': heading,
                                'deviceID': deviceId  // Device ID 추가
                            }
                        }]
                    });

                    // 현재 장치가 KD Tree 모드이고 경로가 보이는 경우에만 스냅된 경로 업데이트
                    if (useKdTree && currentDevice && currentDevice.isPathVisible) {
                        updateSnappedPath(snappedCoordinates, deviceId);
                    }
                } else {
                    console.warn("No nearest point found in KD Tree.");
                }
            } else {
                // Real GPS Path: 실제 GPS 좌표로 지도 위 차량 위치 업데이트
                vehicleSource.setData({
                    'type': 'FeatureCollection',
                    'features': [{
                        'type': 'Feature',
                        'geometry': {
                            'type': 'Point',
                            'coordinates': coordinates
                        },
                        'properties': {
                            'heading': heading,
                            'deviceID': deviceId  // Device ID 추가
                        }
                    }]
                });
            }

            // 상태 업데이트 (스냅된 좌표 또는 실제 좌표로 업데이트)
            if (useKdTree) {
                if (vehicleId === 0) {
                    vehicleLongitude0 = snappedCoordinates[0];
                    vehicleLatitude0 = snappedCoordinates[1];
                } else if (vehicleId === 1) {
                    vehicleLongitude1 = snappedCoordinates[0];
                    vehicleLatitude1 = snappedCoordinates[1];
                }
            } else {
                if (vehicleId === 0) {
                    vehicleLongitude0 = coordinates[0];
                    vehicleLatitude0 = coordinates[1];
                } else if (vehicleId === 1) {
                    vehicleLongitude1 = coordinates[0];
                    vehicleLatitude1 = coordinates[1];
                }
            }

            // 이전 좌표 업데이트 (현재 좌표를 다음에 사용하기 위해 저장)
            previousCoordinatesMap[vehicleId] = snappedCoordinates;

            // 전역 Auto Track 장치가 현재 장치와 같으면 지도 중심 이동
            if (globalAutoTrackDevice && String(globalAutoTrackDevice.id) === String(deviceId)) {
                map.flyTo({
                    center: snappedCoordinates,
                    essential: true
                });
            }

            // 현재 장치의 경로가 보이는 경우에만 GPS 경로 업데이트
            if (currentDevice && currentDevice.isPathVisible) {
                updateGpsPath(coordinates, deviceId); // 실제 GPS 좌표를 경로로 업데이트 (장치별)
            }


        }

        // 장치별 GPS 좌표를 추가하는 함수
        function updateGpsPath(coordinate, deviceId) {
            const sourceId = `gps-path-${deviceId}`;
            const layerId = `gps-path-layer-${deviceId}`;
            const device = activeDevices.get(String(deviceId));
            if (!device || !device.isPathVisible) return;
            if (!window.devicePathData.has(String(deviceId))) {
                window.devicePathData.set(String(deviceId), {
                    gpsPathX: new Float32Array(MAX_PATH_POINTS),
                    gpsPathY: new Float32Array(MAX_PATH_POINTS),
                    gpsPathIndex: 0,
                    snappedPathX: new Float32Array(MAX_PATH_POINTS),
                    snappedPathY: new Float32Array(MAX_PATH_POINTS),
                    snappedPathIndex: 0
                });
            }
            const pathData = window.devicePathData.get(String(deviceId));
            if (pathData.gpsPathIndex < MAX_PATH_POINTS) {
                pathData.gpsPathX[pathData.gpsPathIndex] = coordinate[0];
                pathData.gpsPathY[pathData.gpsPathIndex] = coordinate[1];
                pathData.gpsPathIndex++;
            }
            if (!map.getSource(sourceId)) {
                map.addSource(sourceId, {
                    'type': 'geojson',
                    'data': {'type': 'FeatureCollection', 'features': []}
                });
            }
            const source = map.getSource(sourceId);
            const data = source._data || { 'type': 'FeatureCollection', 'features': [] };
            const feature = getGeoJsonObject();
            feature.geometry.coordinates = coordinate;
            feature.properties.deviceId = deviceId;
            data.features.push(feature);
            source.setData(data);
            if (!map.getLayer(layerId)) {
                map.addLayer({
                    'id': layerId,
                    'type': 'circle',
                    'source': sourceId,
                    'paint': {'circle-radius': 3, 'circle-color': '#FF0000'}
                });
            }
        }

        /************************************************************/
        /* Update Snapped (KD Tree) Path with Blue Dots */
        /************************************************************/
        function updateSnappedPath(coordinate, deviceId) {
            const sourceId = `snapped-path-${deviceId}`;
            const layerId = `snapped-path-layer-${deviceId}`;
            const device = activeDevices.get(String(deviceId));
            if (!device || !device.isPathVisible) return;
            if (!window.devicePathData.has(String(deviceId))) {
                window.devicePathData.set(String(deviceId), {
                    gpsPathX: new Float32Array(MAX_PATH_POINTS),
                    gpsPathY: new Float32Array(MAX_PATH_POINTS),
                    gpsPathIndex: 0,
                    snappedPathX: new Float32Array(MAX_PATH_POINTS),
                    snappedPathY: new Float32Array(MAX_PATH_POINTS),
                    snappedPathIndex: 0
                });
            }
            const pathData = window.devicePathData.get(String(deviceId));
            if (pathData.snappedPathIndex < MAX_PATH_POINTS) {
                pathData.snappedPathX[pathData.snappedPathIndex] = coordinate[0];
                pathData.snappedPathY[pathData.snappedPathIndex] = coordinate[1];
                pathData.snappedPathIndex++;
            }
            if (!map.getSource(sourceId)) {
                map.addSource(sourceId, {
                    'type': 'geojson',
                    'data': {'type': 'FeatureCollection', 'features': []}
                });
            }
            const source = map.getSource(sourceId);
            const data = source._data || { 'type': 'FeatureCollection', 'features': [] };
            const feature = getGeoJsonObject();
            feature.geometry.coordinates = coordinate;
            feature.properties.deviceId = deviceId;
            data.features.push(feature);
            source.setData(data);
            if (!map.getLayer(layerId)) {
                map.addLayer({
                    'id': layerId,
                    'type': 'circle',
                    'source': sourceId,
                    'paint': {'circle-radius': 3, 'circle-color': '#0000FF'}
                });
            }
        }

        // 차량 정보 박스 제거됨 - 함수 주석처리
        // function updateHeadingInfo(heading) {
        //     const headingText = document.getElementById('heading-text');
        //     const formattedHeading = Math.round(heading).toString().padStart(3, '0');
        //     headingText.innerText = `${formattedHeading}°`;
        // }

        // function updateSpeedInfo(speed) {
        //     const speedValue = document.getElementById('speed-value');
        //     const formattedSpeed = Math.round(speed).toString().padStart(2, '0');
        //     speedValue.innerText = formattedSpeed;
        // }

        function fetchAndUpdate() {
            // KD-Tree가 없어도 디바이스 정보는 업데이트하도록 수정
            if (!tree) {
                console.warn("KD-Tree is not built yet. Device info will be updated without path snapping.");
            }

            const devId0 = parseFloat(s_unRxDevId);  // device ID를 파싱
            const latitude0 = parseFloat(s_nRxLatitude);
            const longitude0 = parseFloat(s_nRxLongitude);
            const heading0 = parseFloat(s_unRxVehicleHeading);
            const speed0 = parseFloat(s_unRxVehicleSpeed);

            const devId1 = parseFloat(s_unTxDevId);  // device ID를 파싱
            const latitude1 = parseFloat(s_nTxLatitude);
            const longitude1 = parseFloat(s_nTxLongitude);
            const heading1 = parseFloat(s_unTxVehicleHeading);
            

            // 간단한 디버그 로그
            // //console.log('[COMM] Rx:', devId0, 'Tx:', devId1, '| Heading Rx:', heading0, 'Tx:', heading1);

            // 실제 패킷 카운트 가져오기 (TxTest 모드와 일반 모드 모두 지원)
            const realPacketCount = parseFloat(s_ulTotalPacketCnt) || 0;
            
            // 통신선 디버그 로그 제거

            if (!isNaN(latitude0) && !isNaN(longitude0) && devId0 > 0) {
                // 헤딩값을 반대로 하여 자동차 이미지 방향 반전
                const reversedHeading0 = reverseHeading(heading0);
                updateVehiclePosition(0, [longitude0, latitude0], reversedHeading0, devId0);  // device ID 전달
                // updateHeadingInfo(heading0);  // 차량 정보 박스 제거됨
                // updateSpeedInfo(speed0);     // 차량 정보 박스 제거됨
                
                // 활성 장치 목록에 Rx 장치 정보 업데이트 (실제 패킷 카운트 사용)
                updateDeviceInfo(devId0, 'OBU', {
                    latitude: latitude0,
                    longitude: longitude0,
                    heading: heading0,
                    speed: speed0,
                    attitude: parseFloat(s_nRxAttitude),
                    swVerL1: s_usRxSwVerL1,
                    swVerL2: s_usRxSwVerL2,
                    hwVerL1: s_usRxHwVerL1,
                    hwVerL2: s_usRxHwVerL2,
                    distance: parseFloat(s_usCommDistance),
                    rssi: parseFloat(s_nRssi),
                    rcpi: parseFloat(s_ucRcpi),
                    role: 'Receiver',
                    realPacketCount: realPacketCount,
                    // CAN 값들 추가
                    ...(window.lastCanValues || {})
                });
                
                // V2V 통신 쌍 기록 (Rx가 데이터를 받았다는 것은 Tx와 통신했다는 의미)
                if (!isNaN(devId1) && devId1 > 0) {
                    window.recordCommunicationPair(devId0, devId1, 'V2V');
                }
            }

            if (!isNaN(latitude1) && !isNaN(longitude1)) {
                // 헤딩값을 반대로 하여 자동차 이미지 방향 반전
                const reversedHeading1 = reverseHeading(heading1);
                updateVehiclePosition(1, [longitude1, latitude1], reversedHeading1, devId1);  // device ID 전달
                
                // Tx 장치는 실제 통신하는 경우에만 실제 패킷 카운트 사용 (TxTest 모드 제외)
                const txUpdateInfo = {
                    latitude: latitude1,
                    longitude: longitude1,
                    heading: heading1,
                    attitude: parseFloat(s_nTxAttitude),
                    speed: parseFloat(s_unTxVehicleSpeed),
                    swVerL1: s_usTxSwVerL1,
                    swVerL2: s_usTxSwVerL2,
                    hwVerL1: s_usTxHwVerL1,
                    hwVerL2: s_usTxHwVerL2,
                    role: 'Transmitter',
                    // CAN 값들 추가
                    ...(window.lastCanValues || {})
                };
                
                // Tx 장치가 실제 데이터를 보내는 경우에만 실제 패킷 카운트 사용
                if (!isNaN(devId1) && devId1 > 0) {
                    txUpdateInfo.realPacketCount = realPacketCount;
                }
                
                updateDeviceInfo(devId1, 'OBU', txUpdateInfo);
                
                // V2I 통신 쌍 기록 (RSU와의 통신)
                // 실제 RSU 통신 데이터가 있다면 여기서 기록
                // 현재는 RSU 통신 데이터가 없으므로 주석 처리
                // const rsuId = parseFloat(s_rsuDeviceId); // RSU 장치 ID
                // if (!isNaN(rsuId) && rsuId > 0) {
                //     window.recordCommunicationPair(devId1, rsuId, 'V2I');
                // }
            }
            
            // 통신선 업데이트
            if (globalCommunicationLineVisible) {
                updateCommunicationLineData();
            }
        }

        // 그래프 버튼 업데이트 함수
        function updateGraphButtons(prrValue, latencyValue, rssiValue, rcpiValue) {
            // 성능 등급 평가 함수들
            function getPrrGrade(value) {
                if (value >= 99.0) return { grade: 'A+', color: '#00FF00', icon: '🟢' }; // 우수
                if (value >= 97.0) return { grade: 'A', color: '#90EE90', icon: '🟢' }; // 양호
                if (value >= 95.0) return { grade: 'B', color: '#FFFF00', icon: '🟡' }; // 보통
                if (value >= 93.0) return { grade: 'C', color: '#FFA500', icon: '🟠' }; // 미흡
                return { grade: 'D', color: '#FF0000', icon: '🔴' }; // 불량
            }
            
            function getLatencyGrade(value) {
                if (value <= 1.0) return { grade: 'A+', color: '#00FF00', icon: '🟢' }; // 우수
                if (value <= 2.0) return { grade: 'A', color: '#90EE90', icon: '🟢' }; // 양호
                if (value <= 3.0) return { grade: 'B', color: '#FFFF00', icon: '🟡' }; // 보통
                if (value <= 4.0) return { grade: 'C', color: '#FFA500', icon: '🟠' }; // 미흡
                return { grade: 'D', color: '#FF0000', icon: '🔴' }; // 불량
            }
            
            function getRssiGrade(value) {
                if (value >= -50) return { grade: 'A+', color: '#00FF00', icon: '🟢' }; // 우수
                if (value >= -60) return { grade: 'A', color: '#90EE90', icon: '🟢' }; // 양호
                if (value >= -70) return { grade: 'B', color: '#FFFF00', icon: '🟡' }; // 보통
                if (value >= -80) return { grade: 'C', color: '#FFA500', icon: '🟠' }; // 미흡
                return { grade: 'D', color: '#FF0000', icon: '🔴' }; // 불량
            }
            
            function getRcpiGrade(value) {
                if (value >= -50) return { grade: 'A+', color: '#00FF00', icon: '🟢' }; // 우수
                if (value >= -60) return { grade: 'A', color: '#90EE90', icon: '🟢' }; // 양호
                if (value >= -70) return { grade: 'B', color: '#FFFF00', icon: '🟡' }; // 보통
                if (value >= -80) return { grade: 'C', color: '#FFA500', icon: '🟠' }; // 미흡
                return { grade: 'D', color: '#FF0000', icon: '🔴' }; // 불량
            }
            
            // PRR 값 처리
            if (!isNaN(prrValue)) {
                prrValues.push(prrValue);
                if (prrValues.length > 100) {
                    prrValues.shift(); // 오래된 값 제거
                }
                
                // 전체 PRR 데이터에서 평균 계산 (메모리 효율적 처리)
                let prrAverage = 0;
                if (prrDataBuffer.index > 0 && prrDataBuffer.y) {
                    let sum = 0;
                    for (let i = 0; i < prrDataBuffer.index; i++) {
                        sum += prrDataBuffer.y[i];
                    }
                    prrAverage = sum / prrDataBuffer.index;
                } else if (prrValues.length > 0) {
                    prrAverage = prrValues.reduce((sum, val) => sum + val, 0) / prrValues.length;
                }
                const prrGrade = getPrrGrade(prrValue);
                const prrAvgGrade = getPrrGrade(prrAverage);
                
                // PRR 버튼 업데이트
                const prrCurrentElement = document.getElementById('prr-current');
                const prrAverageElement = document.getElementById('prr-average');
                
                prrCurrentElement.textContent = `${prrValue.toFixed(1)}%`;
                prrCurrentElement.style.color = prrGrade.color;
                prrCurrentElement.title = `현재 PRR: ${prrValue.toFixed(1)}%\n성능 등급: ${prrGrade.grade} (${prrGrade.icon})\n${prrGrade.grade === 'A+' ? '우수' : prrGrade.grade === 'A' ? '양호' : prrGrade.grade === 'B' ? '보통' : prrGrade.grade === 'C' ? '미흡' : '불량'}`;
                
                prrAverageElement.textContent = `${prrAverage.toFixed(1)}%`;
                prrAverageElement.style.color = prrAvgGrade.color;
                prrAverageElement.title = `평균 PRR: ${prrAverage.toFixed(1)}%\n성능 등급: ${prrAvgGrade.grade} (${prrAvgGrade.icon})\n${prrAvgGrade.grade === 'A+' ? '우수' : prrAvgGrade.grade === 'A' ? '양호' : prrAvgGrade.grade === 'B' ? '보통' : prrAvgGrade.grade === 'C' ? '미흡' : '불량'}`;
            } else {
                document.getElementById('prr-current').textContent = '--%';
                document.getElementById('prr-current').style.color = 'white';
                document.getElementById('prr-current').title = '';
                document.getElementById('prr-average').textContent = '--%';
                document.getElementById('prr-average').style.color = 'white';
                document.getElementById('prr-average').title = '';
            }
            
            // Latency 값 처리 - 그래프와 동일한 필터링과 변환 적용
            if (isValidLatency(latencyValue)) {
                // us를 ms로 변환
                const latencyMs = latencyValue / 1000;
                
                latencyValues.push(latencyMs);
                
                // 전체 Latency 데이터에서 평균 계산 (그래프와 동일하게)
                const latencyAverage = latencyData.length > 0 ? 
                    latencyData.reduce((sum, point) => sum + point.y, 0) / latencyData.length : 
                    latencyValues.reduce((sum, val) => sum + val, 0) / latencyValues.length;
                const latencyGrade = getLatencyGrade(latencyMs);
                const latencyAvgGrade = getLatencyGrade(latencyAverage);
                
                // Latency 버튼 업데이트
                const latencyCurrentElement = document.getElementById('latency-current');
                const latencyAverageElement = document.getElementById('latency-average');
                
                latencyCurrentElement.textContent = `${latencyMs.toFixed(1)}ms`;
                latencyCurrentElement.style.color = latencyGrade.color;
                latencyCurrentElement.title = `현재 Latency: ${latencyMs.toFixed(1)}ms\n성능 등급: ${latencyGrade.grade} (${latencyGrade.icon})\n${latencyGrade.grade === 'A+' ? '우수' : latencyGrade.grade === 'A' ? '양호' : latencyGrade.grade === 'B' ? '보통' : latencyGrade.grade === 'C' ? '미흡' : '불량'}`;
                
                latencyAverageElement.textContent = `${latencyAverage.toFixed(1)}ms`;
                latencyAverageElement.style.color = latencyAvgGrade.color;
                latencyAverageElement.title = `평균 Latency: ${latencyAverage.toFixed(1)}ms\n성능 등급: ${latencyAvgGrade.grade} (${latencyAvgGrade.icon})\n${latencyAvgGrade.grade === 'A+' ? '우수' : latencyAvgGrade.grade === 'A' ? '양호' : latencyAvgGrade.grade === 'B' ? '보통' : latencyAvgGrade.grade === 'C' ? '미흡' : '불량'}`;
            } else {
                // 현재값이 유효하지 않을 때는 현재값만 '--'로 표시하고, 평균값은 이전 유효한 평균을 유지
                document.getElementById('latency-current').textContent = '--ms';
                document.getElementById('latency-current').style.color = 'white';
                document.getElementById('latency-current').title = '';
                
                // 이전에 유효한 평균값이 있으면 계속 표시
                if (latencyValues.length > 0) {
                    // 전체 Latency 데이터에서 평균 계산 (그래프와 동일하게)
                    const latencyAverage = latencyData.length > 0 ? 
                        latencyData.reduce((sum, point) => sum + point.y, 0) / latencyData.length : 
                        latencyValues.reduce((sum, val) => sum + val, 0) / latencyValues.length;
                    const latencyAvgGrade = getLatencyGrade(latencyAverage);
                    
                    const latencyAverageElement = document.getElementById('latency-average');
                    latencyAverageElement.textContent = `${latencyAverage.toFixed(1)}ms`;
                    latencyAverageElement.style.color = latencyAvgGrade.color;
                    latencyAverageElement.title = `평균 Latency: ${latencyAverage.toFixed(1)}ms\n성능 등급: ${latencyAvgGrade.grade} (${latencyAvgGrade.icon})\n${latencyAvgGrade.grade === 'A+' ? '우수' : latencyAvgGrade.grade === 'A' ? '양호' : latencyAvgGrade.grade === 'B' ? '보통' : latencyAvgGrade.grade === 'C' ? '미흡' : '불량'}`;
                } else {
                    // 아직 유효한 값이 없으면 평균값도 '--'로 표시
                    document.getElementById('latency-average').textContent = '--ms';
                    document.getElementById('latency-average').style.color = 'white';
                    document.getElementById('latency-average').title = '';
                }
            }
            
            // RSSI 값 처리
            if (!isNaN(rssiValue)) {
                rssiValues.push(rssiValue);
                
                // 전체 RSSI 데이터에서 평균 계산 (그래프와 동일하게)
                const rssiAverage = rssiData.length > 0 ? 
                    rssiData.reduce((sum, point) => sum + point.y, 0) / rssiData.length : 
                    rssiValues.reduce((sum, val) => sum + val, 0) / rssiValues.length;
                const rssiGrade = getRssiGrade(rssiValue);
                const rssiAvgGrade = getRssiGrade(rssiAverage);
                
                // RSSI 버튼 업데이트
                const rssiCurrentElement = document.getElementById('rssi-current');
                const rssiAverageElement = document.getElementById('rssi-average');
                
                rssiCurrentElement.textContent = `${rssiValue.toFixed(1)}dBm`;
                rssiCurrentElement.style.color = rssiGrade.color;
                rssiCurrentElement.title = `현재 RSSI: ${rssiValue.toFixed(1)}dBm\n성능 등급: ${rssiGrade.grade} (${rssiGrade.icon})\n${rssiGrade.grade === 'A+' ? '우수' : rssiGrade.grade === 'A' ? '양호' : rssiGrade.grade === 'B' ? '보통' : rssiGrade.grade === 'C' ? '미흡' : '불량'}`;
                
                rssiAverageElement.textContent = `${rssiAverage.toFixed(1)}dBm`;
                rssiAverageElement.style.color = rssiAvgGrade.color;
                rssiAverageElement.title = `평균 RSSI: ${rssiAverage.toFixed(1)}dBm\n성능 등급: ${rssiAvgGrade.grade} (${rssiAvgGrade.icon})\n${rssiAvgGrade.grade === 'A+' ? '우수' : rssiAvgGrade.grade === 'A' ? '양호' : rssiAvgGrade.grade === 'B' ? '보통' : rssiAvgGrade.grade === 'C' ? '미흡' : '불량'}`;
            }
            else {
                document.getElementById('rssi-current').textContent = '--dBm';
                document.getElementById('rssi-current').style.color = 'white';
                document.getElementById('rssi-current').title = '';
                document.getElementById('rssi-average').textContent = '--dBm';
                document.getElementById('rssi-average').style.color = 'white';
                document.getElementById('rssi-average').title = '';
            }
            
            // RCPI 값 처리 - 변환 공식: (RCPI 값 / 2) - 110
            if (!isNaN(rcpiValue)) {
                // RCPI 값을 dBm으로 변환
                const rcpiDbm = (rcpiValue / 2) - 110;
                
                rcpiValues.push(rcpiDbm);
                
                // 전체 RCPI 데이터에서 평균 계산 (그래프와 동일하게)
                const rcpiAverage = rcpiData.length > 0 ? 
                    rcpiData.reduce((sum, point) => sum + point.y, 0) / rcpiData.length : 
                    rcpiValues.reduce((sum, val) => sum + val, 0) / rcpiValues.length;
                const rcpiGrade = getRcpiGrade(rcpiDbm);
                const rcpiAvgGrade = getRcpiGrade(rcpiAverage);
                
                // RCPI 버튼 업데이트
                const rcpiCurrentElement = document.getElementById('rcpi-current');
                const rcpiAverageElement = document.getElementById('rcpi-average');
                
                rcpiCurrentElement.textContent = `${rcpiDbm.toFixed(1)}dBm`;
                rcpiCurrentElement.style.color = rcpiGrade.color;
                rcpiCurrentElement.title = `현재 RCPI: ${rcpiDbm.toFixed(1)}dBm\n성능 등급: ${rcpiGrade.grade} (${rcpiGrade.icon})\n${rcpiGrade.grade === 'A+' ? '우수' : rcpiGrade.grade === 'A' ? '양호' : rcpiGrade.grade === 'B' ? '보통' : rcpiGrade.grade === 'C' ? '미흡' : '불량'}`;
                
                rcpiAverageElement.textContent = `${rcpiAverage.toFixed(1)}dBm`;
                rcpiAverageElement.style.color = rcpiAvgGrade.color;
                rcpiAverageElement.title = `평균 RCPI: ${rcpiAverage.toFixed(1)}dBm\n성능 등급: ${rcpiAvgGrade.grade} (${rcpiAvgGrade.icon})\n${rcpiAvgGrade.grade === 'A+' ? '우수' : rcpiAvgGrade.grade === 'A' ? '양호' : rcpiAvgGrade.grade === 'B' ? '보통' : rcpiAvgGrade.grade === 'C' ? '미흡' : '불량'}`;
            } else {
                document.getElementById('rcpi-current').textContent = '--dBm';
                document.getElementById('rcpi-current').style.color = 'white';
                document.getElementById('rcpi-current').title = '';
                document.getElementById('rcpi-average').textContent = '--dBm';
                document.getElementById('rcpi-average').style.color = 'white';
                document.getElementById('rcpi-average').title = '';
            }
        }

        /************************************************************/
        /* Graph */
        /************************************************************/
        function fetchAndUpdateGraph() {
            const unPdr = parseFloat(s_unPdr);
            const ulLatencyL1 = parseFloat(s_ulLatencyL1);
            const ulTotalPacketCnt = parseFloat(s_ulTotalPacketCnt);
            const nRxLatitude = parseFloat(s_nRxLatitude);
            const nRxLongitude = parseFloat(s_nRxLongitude);
            const nRxAttitude = parseFloat(s_nRxAttitude);
            const nTxLatitude = parseFloat(s_nTxLatitude);
            const nTxLongitude = parseFloat(s_nTxLongitude);
            const nTxAttitude = parseFloat(s_nTxAttitude);
            const usCommDistance = parseFloat(s_usCommDistance);
            const nRssi = parseFloat(s_nRssi);
            const ucRcpi = parseFloat(s_ucRcpi);
            const devId0 = parseFloat(s_unRxDevId);
            const devId1 = parseFloat(s_unTxDevId);
            const usTxSwVerL1 = parseFloat(s_usTxSwVerL1);
            const usTxSwVerL2 = parseFloat(s_usTxSwVerL2);
            const usRxSwVerL1 = parseFloat(s_usRxSwVerL1);
            const usRxSwVerL2 = parseFloat(s_usRxSwVerL2);
            const usTxHwVerL1 = parseFloat(s_usTxHwVerL1);
            const usTxHwVerL2 = parseFloat(s_usTxHwVerL2);
            const usRxHwVerL1 = parseFloat(s_usRxHwVerL1);
            const usRxHwVerL2 = parseFloat(s_usRxHwVerL2);

            // 선택된 장치가 있으면 선택된 장치 센서값으로 업데이트
            if (selectedDevice) {
                updateSensorValuesForSelectedDevice();
            }

            // 실제 값 그대로 사용 (가짜 데이터 생성 제거)
            const refinedPdr = unPdr;
            const refinedLatency = ulLatencyL1;
            // PRR 값 저장 (Rx-Tx 쌍)
            if (!isNaN(devId0) && !isNaN(devId1) && devId0 > 0 && devId1 > 0) {
                const pairKey = `${Math.min(devId0, devId1)}-${Math.max(devId0, devId1)}`;
                communicationPairPRR.set(pairKey, refinedPdr);
            }

            // 버튼에 현재값과 평균값 업데이트
            updateGraphButtons(refinedPdr, refinedLatency, nRssi, ucRcpi);

            if (!isNaN(refinedPdr) && !isNaN(ulTotalPacketCnt)) {
                updateGraph1(ulTotalPacketCnt, refinedPdr);
            } else {
                console.error('Invalid data points for Graph1.');
            }

            // CSV 저장 기능 호출 - 주석처리
            /*
            const timestamp = new Date().toISOString();
            let latencyToSave = isValidLatency(refinedLatency) ? refinedLatency : '';
            saveToCSV(
                timestamp,
                ulTotalPacketCnt,
                refinedPdr,
                latencyToSave,
                devId1, // Tx Device ID
                devId0, // Rx Device ID
                usCommDistance,
                nRssi,
                ucRcpi
            );
            */
            
            if (isValidLatency(refinedLatency) && !isNaN(ulTotalPacketCnt)) {
                updateGraph2(ulTotalPacketCnt, refinedLatency);
            } else {
                // 레이턴시가 비정상일 때는 그래프2, latency-value 텍스트 업데이트 안 함
                // document.getElementById('latency-value').innerText = 'Latency (Air to Air) -';
            }
            
            // RSSI 그래프 업데이트
            if (!isNaN(nRssi) && !isNaN(ulTotalPacketCnt)) {
                updateGraph3(ulTotalPacketCnt, nRssi);
            }
            
            // RCPI 그래프 업데이트
            if (!isNaN(ucRcpi) && !isNaN(ulTotalPacketCnt)) {
                updateGraph4(ulTotalPacketCnt, ucRcpi);
            }
        }

        // CSV 초기화 - 주석처리
        /*
        initializeCSV();
        updateCSVDataCount();
        */
        
        // Liquid Glass 슬라이더 설정
        setupLiquidGlassSlider();
        
        fetchAndUpdateGraph();
        setInterval(fetchAndUpdateGraph, 100); // 더 빠른 실시간 업데이트
        


        function updateGraph1(xValue, unPdrValue) {
            if (!Array.isArray(xValue)) {
                xValue = [xValue];
            }
            if (!Array.isArray(unPdrValue)) {
                unPdrValue = [unPdrValue];
            }

            if (!isNaN(xValue[0]) && !isNaN(unPdrValue[0])) {
                // 데이터 저장 (메모리 최적화 적용 - 동적 버퍼 관리)
                const requiredSize = prrDataBuffer.index + 1;
                prrDataBuffer.ensureCapacity(Math.max(requiredSize, 10000));
                
                prrDataBuffer.x[prrDataBuffer.index] = xValue[0];
                prrDataBuffer.y[prrDataBuffer.index] = unPdrValue[0];
                prrDataBuffer.index++;
                

                // 전체 PRR 데이터와 선택된 범위 PRR 데이터 동시 업데이트
                updateAllGraphs();

                Plotly.relayout('prr-chart-area', {
                    yaxis: {
                        range: [94, 100],
                        title: 'PRR(%)',
                        dtick: 0.1,
                        tickmode: 'array',
                        tickvals: [94, 95, 96, 97, 98, 99, 100],
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    xaxis: {
                        range : [Math.max(0, xValue[0] - 500), xValue[0]],
                        title: 'Total Packets',
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    }
                });

                // 그래프 우상단 현재값 업데이트는 제거됨 (HTML 요소 삭제)
            } else {
                console.error('Invalid data points for Graph1.');
            }
        }

        // 전체 그래프 업데이트 함수 (노란선과 초록선 동시 업데이트 - 메모리 최적화)
        function updateAllGraphs() {
            if (prrDataBuffer.index === 0) return;

            // 메모리 효율적 최대값 계산
            let latestX = 0;
            for (let i = 0; i < prrDataBuffer.index; i++) {
                if (prrDataBuffer.x[i] > latestX) {
                    latestX = prrDataBuffer.x[i];
                }
            }
            
            const visibleRangeStart = Math.max(0, latestX - 500);
            const visibleRangeEnd = latestX;
            
            // 공통 X축 포인트 생성 (500개)
            const commonX = [];
            const targetLength = 500;
            
            for (let i = 0; i < targetLength; i++) {
                const ratio = i / (targetLength - 1);
                const xValue = visibleRangeStart + (visibleRangeEnd - visibleRangeStart) * ratio;
                commonX.push(xValue);
            }

            // 노란선 (전체 PRR 데이터) 업데이트 - 메모리 효율적 처리
            const currentVisibleData = [];
            for (let i = 0; i < prrDataBuffer.index; i++) {
                if (prrDataBuffer.x[i] >= visibleRangeStart && prrDataBuffer.x[i] <= visibleRangeEnd) {
                    currentVisibleData.push({x: prrDataBuffer.x[i], y: prrDataBuffer.y[i]});
                }
            }
            
            const yellowLineY = [];
            if (currentVisibleData.length > 0) {
                for (let i = 0; i < targetLength; i++) {
                    const dataRatio = (currentVisibleData.length > 1) ? (i / (targetLength - 1)) : 0;
                    const dataIndex = Math.floor(dataRatio * (currentVisibleData.length - 1));
                    const safeIndex = Math.max(0, Math.min(dataIndex, currentVisibleData.length - 1));
                    yellowLineY.push(currentVisibleData[safeIndex].y);
                }
            }

            // 초록선 (선택된 범위 PRR) 업데이트
            let rangeStart, rangeEnd;
            let selectedData;
            
            if (rangeSize === 0) {
                // 크기가 0이면 전체 범위 사용
                selectedData = [];
                for (let i = 0; i < prrDataBuffer.index; i++) {
                    selectedData.push({x: prrDataBuffer.x[i], y: prrDataBuffer.y[i]});
                }
                rangeStart = prrDataBuffer.index > 0 ? Math.min(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index))) : 0;
                rangeEnd = prrDataBuffer.index > 0 ? Math.max(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index))) : 0;
            } else {
                if (isFollowingLatest) {
                    rangeEnd = latestX;
                    rangeStart = Math.max(0, latestX - rangeSize);
                } else {
                    const slider = document.getElementById('prr-range-track');
                    if (slider) {
                        rangeStart = parseInt(slider.dataset.rangeStart) || 0;
                        rangeEnd = rangeStart + rangeSize;
                    }
                }
                
                selectedData = [];
                for (let i = 0; i < prrDataBuffer.index; i++) {
                    if (prrDataBuffer.x[i] >= rangeStart && prrDataBuffer.x[i] <= rangeEnd) {
                        selectedData.push({x: prrDataBuffer.x[i], y: prrDataBuffer.y[i]});
                    }
                }
            }

            const greenLineY = [];
            const originalX = [];
            
            if (selectedData.length > 0) {
                // 선택된 데이터를 x축 전체에 고르게 분포시킴
                for (let i = 0; i < targetLength; i++) {
                    const dataRatio = (selectedData.length > 1) ? (i / (targetLength - 1)) : 0;
                    const dataIndex = Math.floor(dataRatio * (selectedData.length - 1));
                    const safeIndex = Math.max(0, Math.min(dataIndex, selectedData.length - 1));
                    greenLineY.push(selectedData[safeIndex].y);
                    originalX.push(selectedData[safeIndex].x);
                }
            }

            // 두 선 동시 업데이트
            if (yellowLineY.length > 0) {
                Plotly.restyle('prr-chart-area', {
                    x: [commonX, selectedData.length > 0 ? commonX : []],
                    y: [yellowLineY, greenLineY],
                    customdata: [undefined, selectedData.length > 0 ? originalX.map(x => [x]) : []]
                }, [0, 1]);
            }

            // 슬라이더 UI 업데이트
            updateSliderUI(rangeStart, rangeEnd);
        }

        // 선택된 범위 업데이트 함수 (기존 함수를 간소화)
        function updateSelectedRangeGraph() {
            // 전체 그래프 업데이트 함수 호출
            updateAllGraphs();
        }

        // 슬라이더 UI 업데이트 함수
        function updateSliderUI(rangeStart, rangeEnd) {
            const rangeText = document.getElementById('prr-range-text');
            if (rangeText) {
                const sizeText = rangeSize === 0 ? '전체' : rangeSize.toString();
                rangeText.textContent = `범위: ${rangeStart} ~ ${rangeEnd} 패킷 (크기: ${sizeText})`;
            }

            // 슬라이더 위치 업데이트
            const slider = document.getElementById('prr-range-track');
            const selectedRange = document.getElementById('prr-selected-range');
            if (slider && selectedRange && prrDataBuffer.index > 0) {
                if (rangeSize === 0) {
                    // 전체 범위일 때 슬라이더 숨김
                    selectedRange.style.display = 'none';
                } else {
                    selectedRange.style.display = 'block';
                    const maxX = Math.max(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                    const minX = Math.min(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                    const totalRange = maxX - minX;
                    
                    if (totalRange > 0) {
                        const startPercent = ((rangeStart - minX) / totalRange) * 100;
                        const widthPercent = (rangeSize / totalRange) * 100;
                        
                        selectedRange.style.left = Math.max(0, Math.min(100 - widthPercent, startPercent)) + '%';
                        selectedRange.style.width = Math.min(100, widthPercent) + '%';
                    }
                }
            }
        }

        let latencyData = [];

        // CSV 저장을 위한 함수들 - 주석처리
        /*
        function initializeCSV() {
            const now = new Date();
            const timestamp = now.getFullYear() + 
                            String(now.getMonth() + 1).padStart(2, '0') + 
                            String(now.getDate()).padStart(2, '0') + '_';
                            String(now.getHours()).padStart(2, '0') + 
                            String(now.getMinutes()).padStart(2, '0') + 
                            String(now.getSeconds()).padStart(2, '0');
            
            // IP 주소 정보 가져오기
            const urlParams = new URLSearchParams(window.location.search);
            const ipAddress = urlParams.get('ip') || 'unknown';
            
            globalCsvFileName = `v2x_performance_data_${ipAddress}_${timestamp}.csv`;
            
            // CSV 헤더 추가
            globalCsvData.push(['Timestamp', 'TotalRxPackets', 'PRR(%)', 'Latency(μs)', 'Latency(ms)', 'TxDeviceID', 'RxDeviceID', 'Distance(m)', 'RSSI(dBm)', 'RCPI(dBm)']);
            
            //console.log(`CSV 파일 초기화 완료: ${globalCsvFileName}`);
        }

        function saveToCSV(timestamp, totalRxPackets, prr, latency, txDeviceId, rxDeviceId, distance, rssi, rcpi) {
            const latencyMs = (latency / 1000).toFixed(3);
            const row = [
                timestamp,
                totalRxPackets,
                prr.toFixed(2),
                latency,
                latencyMs,
                txDeviceId || 'N/A',
                rxDeviceId || 'N/A',
                distance ? distance.toFixed(2) : 'N/A',
                rssi || 'N/A',
                rcpi || 'N/A'
            ];
            
            globalCsvData.push(row);
            // //console.log(`데이터 추가됨: ${globalCsvData.length}개 행`); // 삭제
            // CSV 데이터 개수 업데이트
            updateCSVDataCount();
        }
        */



        function updateGraph2(xValue, ulLatencyValue) {
            if (!Array.isArray(xValue)) {
                xValue = [xValue];
            }
            if (!Array.isArray(ulLatencyValue)) {
                ulLatencyValue = [ulLatencyValue];
            }

            if (!isNaN(xValue[0]) && !isNaN(ulLatencyValue[0])) {
                // μs를 ms로 변환하여 저장
                const latencyMs = ulLatencyValue[0] / 1000;
                latencyData.push({x: xValue[0], y: latencyMs});

                Plotly.update('latency-chart-area', {
                    x: [latencyData.map(point => point.x)],
                    y: [latencyData.map(point => point.y)]
                }, [0]);

                let avgLatency = latencyData.reduce((sum, point) => sum + point.y, 0) / latencyData.length;

                Plotly.relayout('latency-chart-area', {
                    yaxis: {
                        range: [0, 5],
                        title: 'Latency (ms)',
                        dtick: 0.1,
                        tickmode: 'array',
                        tickvals: [0, 1, 2, 3, 4, 5],
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    xaxis: {
                        range : [Math.max(0, xValue[0] - 500), xValue[0]],
                        title: 'Total Packets',
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    shapes: [
                        {
                            type: 'line',
                            x0: latencyData[0].x, x1: latencyData[latencyData.length - 1].x,
                            y0: avgLatency, y1: avgLatency,
                            line: {
                                color: '#FFD700',
                                width: 1,
                                dash: 'dash'
                            }
                        }
                    ],
                    annotations: [
                        {
                            x: 0.05,
                            y: 0.95,
                            xref: 'paper',
                            yref: 'paper',
                            text: `Avg: ${avgLatency.toFixed(3)} ms`,
                            showarrow: false,
                            font: {
                                family: 'Arial, sans-serif',
                                size: 12,
                                color: 'rgba(255, 255, 255, 0.9)',
                            },
                            align: 'left',
                            xanchor: 'left',
                            yanchor: 'top',
                            bordercolor: 'rgba(255, 215, 0, 0.6)',
                            borderwidth: 1,
                            borderpad: 4,
                            bgcolor: 'rgba(255, 255, 224, 0.1)',
                            opacity: 0.9
                        }
                    ]
                });

                // 그래프 우상단 현재값 업데이트는 제거됨 (HTML 요소 삭제)
            } else {
                console.error('Invalid data points for Graph2.');
            }
        }

        const graph1 = Plotly.newPlot('prr-chart-area', [{
            x: [],
            y: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: '#FFD700', width: 1 },
            marker: { color: '#FFD700', size: 2 },
            name: '전체 PRR',
            hovertemplate: '<span style="color:#FFD700">●</span> <b>전체 PRR:</b> %{y:.2f}% (패킷: %{x})<extra></extra>',
            cliponaxis: false
        }, {
            x: [],
            y: [],
            customdata: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: '#00FF00', width: 1.5 },
            marker: { color: '#00FF00', size: 2.5 },
            name: '선택 구간 PRR',
            hovertemplate: '<span style="color:#00FF00">●</span> <b>선택 구간:</b> %{y:.2f}% (패킷: %{customdata[0]})<extra></extra>',
            cliponaxis: false
        }], {
            margin: { t: 15, b: 35, l: 55, r: 35 }, // margin 증가
            yaxis: { 
                range: [94, 100], 
                title: 'PRR (%)', 
                showgrid: true, 
                zeroline: true, 
                dtick: 0.1,
                tickmode: 'array',
                tickvals: [94, 95, 96, 97, 98, 99, 100],
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                autorange: false, // 고정 범위 사용
                fixedrange: true // 줌 비활성화
            },
            xaxis: { 
                title: 'Total Packets', 
                showgrid: true,
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                fixedrange: true // 줌 비활성화
            },
            hovermode: 'x unified',
            hoverlabel: {
                bgcolor: 'rgba(0, 0, 0, 0.9)',
                bordercolor: 'rgba(255, 255, 255, 0.3)',
                font: {
                    family: 'Arial, sans-serif',
                    size: 11,
                    color: 'white'
                }
            },
            showlegend: false,
            plot_bgcolor: 'transparent',
            paper_bgcolor: 'transparent',
            font: {
                color: 'rgba(255, 255, 255, 0.8)'
            },
            autosize: true, // 자동 크기 조정


        }, {
            displayModeBar: false,
            responsive: true // 반응형 설정
        });

        Plotly.newPlot('latency-chart-area', [{
            x: [],
            y: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: '#FF7F50', width: 1 },
            marker: { color: '#FF7F50', size: 2 },
            cliponaxis: false
        }], {
            margin: { t: 15, b: 35, l: 55, r: 35 }, // margin 증가
            yaxis: { 
                range: [0, 5], 
                title: 'Latency (ms)', 
                showgrid: true, 
                zeroline: true, 
                dtick: 0.1,
                tickmode: 'array',
                tickvals: [0, 1, 2, 3, 4, 5],
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                autorange: false, // 고정 범위 사용
                fixedrange: true // 줌 비활성화
            },
            xaxis: { 
                title: 'Total Packets', 
                showgrid: true,
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                fixedrange: true // 줌 비활성화
            },
            showlegend: false,
            plot_bgcolor: 'transparent',
            paper_bgcolor: 'transparent',
            font: {
                color: 'rgba(255, 255, 255, 0.8)'
            },
            autosize: true, // 자동 크기 조정

        }, {
            displayModeBar: false,
            responsive: true // 반응형 설정
        });

        // 새로운 Liquid Glass 슬라이더 설정 함수
        function setupLiquidGlassSlider() {
            const sizeInput = document.getElementById('prr-range-size-input');
            const track = document.getElementById('prr-range-track');
            const selectedRange = document.getElementById('prr-selected-range');
            const rangeText = document.getElementById('prr-range-text');

            if (!sizeInput || !track || !selectedRange || !rangeText) {
                console.warn('슬라이더 요소를 찾을 수 없습니다.');
                return;
            }

            // 초기값 설정
            sizeInput.value = rangeSize;
            rangeText.textContent = `범위: 0 ~ ${rangeSize} 패킷 (크기: ${rangeSize})`;

            // 크기 입력 이벤트 설정
            sizeInput.addEventListener('input', (e) => {
                const newSize = parseInt(e.target.value);
                if (!isNaN(newSize) && newSize >= 0) {
                    rangeSize = newSize;
                    // 0이면 자동 모드로 전환
                    if (rangeSize === 0) {
                        isFollowingLatest = true;
                    }
                    updateSelectedRangeGraph();
                }
            });

            sizeInput.addEventListener('change', (e) => {
                const newSize = parseInt(e.target.value);
                if (isNaN(newSize) || newSize < 0) {
                    e.target.value = rangeSize; // 유효하지 않으면 이전 값으로 복원
                } else {
                    rangeSize = newSize;
                    // 0이면 자동 모드로 전환
                    if (rangeSize === 0) {
                        isFollowingLatest = true;
                    }
                    updateSelectedRangeGraph();
                }
            });

            // 이벤트 설정
            setupSliderEvents(track, selectedRange, rangeText);
        }

        // 슬라이더 이벤트 처리 함수
        function setupSliderEvents(track, selectedRange, rangeText) {
            let isDragging = false;
            let startX = 0;
            let startLeft = 0;

            function handleMouseMove(e) {
                if (!isDragging || prrDataBuffer.index === 0 || rangeSize === 0) return;

                const trackRect = track.getBoundingClientRect();
                const deltaX = e.clientX - startX;
                const deltaPercent = (deltaX / trackRect.width) * 100;
                
                let newLeft = startLeft + deltaPercent;
                const rangeWidth = parseFloat(selectedRange.style.width) || 100;
                
                // 범위가 트랙을 벗어나지 않도록 제한
                newLeft = Math.max(0, Math.min(100 - rangeWidth, newLeft));
                
                // 선택된 범위 위치 업데이트
                selectedRange.style.left = newLeft + '%';

                // 데이터 범위 계산
                const maxX = Math.max(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                const minX = Math.min(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                const totalRange = maxX - minX;
                
                if (totalRange > 0) {
                    const rangeStart = Math.round(minX + (newLeft / 100) * totalRange);
                    const rangeEnd = rangeStart + rangeSize;
                    
                    // 슬라이더에 범위 정보 저장
                    const slider = document.getElementById('prr-range-track');
                    if (slider) {
                        slider.dataset.rangeStart = rangeStart;
                    }
                    
                    // 자동 추적 모드 비활성화
                    const latestX = Math.max(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                    isFollowingLatest = Math.abs(rangeEnd - latestX) < 50;
                    
                    // 그래프 업데이트
                    updateSelectedRangeGraph();
                }
            }

            function handleMouseUp() {
                if (isDragging) {
                    isDragging = false;
                    selectedRange.style.cursor = 'grab';
                    document.removeEventListener('mousemove', handleMouseMove);
                    document.removeEventListener('mouseup', handleMouseUp);
                }
            }

            // 드래그 시작
            selectedRange.addEventListener('mousedown', (e) => {
                if (rangeSize === 0) return; // 전체 범위일 때 드래그 비활성화
                
                isDragging = true;
                startX = e.clientX;
                startLeft = parseFloat(selectedRange.style.left) || 0;
                selectedRange.style.cursor = 'grabbing';
                e.preventDefault();
                
                document.addEventListener('mousemove', handleMouseMove);
                document.addEventListener('mouseup', handleMouseUp);
            });

            // 트랙 클릭으로 이동
            track.addEventListener('click', (e) => {
                if (prrDataBuffer.index === 0 || rangeSize === 0) return;
                
                const trackRect = track.getBoundingClientRect();
                const clickPercent = ((e.clientX - trackRect.left) / trackRect.width) * 100;
                const rangeWidth = parseFloat(selectedRange.style.width) || 100;
                
                // 클릭한 위치를 중심으로 배치
                let newLeft = clickPercent - (rangeWidth / 2);
                newLeft = Math.max(0, Math.min(100 - rangeWidth, newLeft));
                
                selectedRange.style.left = newLeft + '%';
                
                // 데이터 범위 계산 및 업데이트
                const maxX = Math.max(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                const minX = Math.min(...Array.from(prrDataBuffer.x.slice(0, prrDataBuffer.index)));
                const totalRange = maxX - minX;
                
                if (totalRange > 0) {
                    const rangeStart = Math.round(minX + (newLeft / 100) * totalRange);
                    
                    // 슬라이더에 범위 정보 저장
                    const slider = document.getElementById('prr-range-track');
                    if (slider) {
                        slider.dataset.rangeStart = rangeStart;
                    }
                    
                    // 자동 추적 모드 비활성화
                    isFollowingLatest = false;
                    
                    // 그래프 업데이트
                    updateSelectedRangeGraph();
                }
            });

            // 슬라이더는 이미 HTML에 존재하므로 ID 설정 불필요
        }

        // cleanup 함수 반환
        return {
            cleanup: () => {
                // 데이터 초기화 (메모리 최적화 적용)
                prrDataBuffer.reset();
                isFollowingLatest = true;
                
                // 모든 장치의 경로 데이터 완전 초기화
                //console.log('cleanup - 모든 장치 경로 데이터 완전 초기화 시작');
                for (const [deviceId, device] of activeDevices) {
                    if (typeof window.clearDevicePathData === 'function') {
                        window.clearDevicePathData(deviceId);
                    }
                }
                
                // 전역 변수들 초기화
                selectedDevice = null;
                globalAutoTrackDevice = null;
                activeDevices.clear();
                
                // KD Tree 사용 여부 초기화
                if (window.deviceKdTreeUsage) {
                    window.deviceKdTreeUsage.clear();
                }
                
                // 저장된 경로 데이터 초기화
                if (window.devicePathData) {
                    window.devicePathData.clear();
                    //console.log('저장된 경로 데이터 초기화 완료');
                }
                
                // 센서 패널 숨기기
                hideSensorPanels();
                
                // 모든 경로 숨기기
                hideAllDevicePaths();
                
                const obuListElement = document.getElementById('obu-list');
                const rsuListElement = document.getElementById('rsu-list');
                const obuCountElement = document.getElementById('obu-count');
                const rsuCountElement = document.getElementById('rsu-count');
                
                if (obuListElement) {
                    obuListElement.innerHTML = '<div class="no-devices">검색된 OBU 장치가 없습니다</div>';
                }
                if (rsuListElement) {
                    rsuListElement.innerHTML = '<div class="no-devices">검색된 RSU 장치가 없습니다</div>';
                }
                if (obuCountElement) {
                    obuCountElement.textContent = '0개';
                }
                if (rsuCountElement) {
                    rsuCountElement.textContent = '0개';
                }
                
                // 메모리 풀 정리
                geoJsonPool.length = 0;
                
                //console.log('cleanup - 모든 데이터 초기화 완료');
            }
        };

    }
};

// 전역 변수들 - 주석처리
/*
let globalCsvData = [];
let globalCsvFileName = '';
let globalAutoSaveInterval = null;
let globalIsAutoSaveEnabled = false;
*/

// 전역 CSV 관련 함수들 - 주석처리
/*
function downloadCSV() {
    if (globalCsvData.length <= 1) {
        alert('저장할 데이터가 없습니다.');
        return;
    }
    
    // CSV 데이터를 문자열로 변환
    const csvContent = globalCsvData.map(row => row.join(',')).join('\n');
    
    // Blob 생성 및 다운로드
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    const link = document.createElement('a');
    
    if (link.download !== undefined) {
        const url = URL.createObjectURL(blob);
        link.setAttribute('href', url);
        link.setAttribute('download', globalCsvFileName);
        link.style.visibility = 'hidden';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
        
        // 사용자에게 피드백 제공
        const downloadButton = document.getElementById('downloadCSVButton');
        if (downloadButton) {
            const originalText = downloadButton.textContent;
            downloadButton.textContent = '다운로드 완료!';
            downloadButton.style.backgroundColor = '#2196F3';
            setTimeout(() => {
                downloadButton.textContent = originalText;
                downloadButton.style.backgroundColor = '#4CAF50';
            }, 2000);
        }
        
        //console.log(`CSV 파일 다운로드 완료: ${globalCsvFileName}`);
    }
}
*/

/*
function clearCSVData() {
    globalCsvData = [globalCsvData[0]]; // 헤더만 유지
    //console.log('CSV 데이터 초기화 완료');
    updateCSVDataCount();
}

function updateCSVDataCount() {
    const dataCount = globalCsvData.length - 1; // 헤더 제외한 데이터 개수
    const csvDataCountElement = document.getElementById('csv-data-count');
    if (csvDataCountElement) {
        csvDataCountElement.innerText = `수집된 데이터: ${dataCount}개`;
    }
}

function toggleAutoSave() {
    globalIsAutoSaveEnabled = !globalIsAutoSaveEnabled;
    
    if (globalIsAutoSaveEnabled) {
        // 자동 저장 시작 (30초마다)
        globalAutoSaveInterval = setInterval(() => {
            if (globalCsvData.length > 1) {
                downloadCSV();
                //console.log('자동 저장 완료');
            }
        }, 30000); // 30초마다
        
        const autoSaveButton = document.getElementById('autoSaveButton');
        if (autoSaveButton) {
            autoSaveButton.textContent = '자동저장 중지';
            autoSaveButton.style.backgroundColor = '#f44336';
        }
        //console.log('자동 저장 시작');
    } else {
        // 자동 저장 중지
        if (globalAutoSaveInterval) {
            clearInterval(globalAutoSaveInterval);
            globalAutoSaveInterval = null;
        }
        
        const autoSaveButton = document.getElementById('autoSaveButton');
        if (autoSaveButton) {
            autoSaveButton.textContent = '자동저장 시작';
            autoSaveButton.style.backgroundColor = '#4CAF50';
        }
        //console.log('자동 저장 중지');
    }
}

function getCSVDataCount() {
    return globalCsvData.length - 1; // 헤더 제외한 데이터 개수
}

// 전역 함수로 노출
window.downloadCSV = downloadCSV;
window.clearCSVData = clearCSVData;
window.toggleAutoSave = toggleAutoSave;
window.getCSVDataCount = getCSVDataCount;
*/

function updateDateTime() {
    const now = new Date();

    const year = now.getFullYear();
    const month = String(now.getMonth() + 1).padStart(2, '0');
    const day = String(now.getDate()).padStart(2, '0');
    const weekDays = ['일', '월', '화', '수', '목', '금', '토'];
    const weekDay = weekDays[now.getDay()];
    const hours = String(now.getHours()).padStart(2, '0');
    const minutes = String(now.getMinutes()).padStart(2, '0');
    const seconds = String(now.getSeconds()).padStart(2, '0');

    const dateTimeString = `${year}년 ${month}월 ${day}일 (${weekDay})      ${hours}시 ${minutes}분 ${seconds}초`;

    document.getElementById('datetime-info').innerText = dateTimeString;
}

// 1초마다 업데이트
setInterval(updateDateTime, 1000);

// 페이지 로드 시 즉시 한 번 실행
updateDateTime();



// 레이턴시 값 유효성 검사 함수
function isValidLatency(value) {
    return typeof value === 'number' && isFinite(value) && value > 0;
}

        let rssiData = [];
        let rcpiData = [];

        function updateGraph3(xValue, rssiValue) {
            if (!Array.isArray(xValue)) {
                xValue = [xValue];
            }
            if (!Array.isArray(rssiValue)) {
                rssiValue = [rssiValue];
            }

            if (!isNaN(xValue[0]) && !isNaN(rssiValue[0])) {
                rssiData.push({x: xValue[0], y: rssiValue[0]});

                Plotly.update('rssi-chart-area', {
                    x: [rssiData.map(point => point.x)],
                    y: [rssiData.map(point => point.y)]
                }, [0]);

                let avgRssi = rssiData.reduce((sum, point) => sum + point.y, 0) / rssiData.length;

                Plotly.relayout('rssi-chart-area', {
                    yaxis: {
                        range: [-100, -30],
                        title: 'RSSI (dBm)',
                        dtick: 10,
                        tickmode: 'array',
                        tickvals: [-100, -90, -80, -70, -60, -50, -40, -30],
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    xaxis: {
                        range : [Math.max(0, xValue[0] - 500), xValue[0]],
                        title: 'Total Packets',
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    shapes: [
                        {
                            type: 'line',
                            x0: rssiData[0].x, x1: rssiData[rssiData.length - 1].x,
                            y0: avgRssi, y1: avgRssi,
                            line: {
                                color: '#FFD700',
                                width: 1,
                                dash: 'dash'
                            }
                        }
                    ],
                    annotations: [
                        {
                            x: 0.05,
                            y: 0.95,
                            xref: 'paper',
                            yref: 'paper',
                            text: `Avg: ${avgRssi.toFixed(1)} dBm`,
                            showarrow: false,
                            font: {
                                family: 'Arial, sans-serif',
                                size: 12,
                                color: 'rgba(255, 255, 255, 0.9)',
                            },
                            align: 'left',
                            xanchor: 'left',
                            yanchor: 'top',
                            bordercolor: 'rgba(255, 215, 0, 0.6)',
                            borderwidth: 1,
                            borderpad: 4,
                            bgcolor: 'rgba(255, 255, 224, 0.1)',
                            opacity: 0.9
                        }
                    ]
                });
            } else {
                console.error('Invalid data points for Graph3 (RSSI).');
            }
        }

        function updateGraph4(xValue, rcpiValue) {
            if (!Array.isArray(xValue)) {
                xValue = [xValue];
            }
            if (!Array.isArray(rcpiValue)) {
                rcpiValue = [rcpiValue];
            }

            if (!isNaN(xValue[0]) && !isNaN(rcpiValue[0])) {
                // RCPI 값을 dBm으로 변환: (RCPI 값 / 2) - 110
                const rcpiDbm = (rcpiValue[0] / 2) - 110;
                rcpiData.push({x: xValue[0], y: rcpiDbm});

                Plotly.update('rcpi-chart-area', {
                    x: [rcpiData.map(point => point.x)],
                    y: [rcpiData.map(point => point.y)]
                }, [0]);

                let avgRcpi = rcpiData.reduce((sum, point) => sum + point.y, 0) / rcpiData.length;

                Plotly.relayout('rcpi-chart-area', {
                    yaxis: {
                        range: [-100, -30],
                        title: 'RCPI (dBm)',
                        dtick: 10,
                        tickmode: 'array',
                        tickvals: [-100, -90, -80, -70, -60, -50, -40, -30],
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    xaxis: {
                        range : [Math.max(0, xValue[0] - 500), xValue[0]],
                        title: 'Total Packets',
                        gridcolor: 'rgba(255, 255, 255, 0.1)',
                        tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                        titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                        autorange: false,
                        fixedrange: true
                    },
                    shapes: [
                        {
                            type: 'line',
                            x0: rcpiData[0].x, x1: rcpiData[rcpiData.length - 1].x,
                            y0: avgRcpi, y1: avgRcpi,
                            line: {
                                color: '#FFD700',
                                width: 1,
                                dash: 'dash'
                            }
                        }
                    ],
                    annotations: [
                        {
                            x: 0.05,
                            y: 0.95,
                            xref: 'paper',
                            yref: 'paper',
                            text: `Avg: ${avgRcpi.toFixed(1)} dBm`,
                            showarrow: false,
                            font: {
                                family: 'Arial, sans-serif',
                                size: 12,
                                color: 'rgba(255, 255, 255, 0.9)',
                            },
                            align: 'left',
                            xanchor: 'left',
                            yanchor: 'top',
                            bordercolor: 'rgba(255, 215, 0, 0.6)',
                            borderwidth: 1,
                            borderpad: 4,
                            bgcolor: 'rgba(255, 255, 224, 0.1)',
                            opacity: 0.9
                        }
                    ]
                });
            } else {
                console.error('Invalid data points for Graph4 (RCPI).');
            }
        }

        Plotly.newPlot('rssi-chart-area', [{
            x: [],
            y: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: '#9370DB', width: 1 },
            marker: { color: '#9370DB', size: 2 },
            cliponaxis: false
        }], {
            margin: { t: 15, b: 35, l: 55, r: 35 }, // margin 증가
            yaxis: { 
                range: [-100, -30], 
                title: 'RSSI (dBm)', 
                showgrid: true, 
                zeroline: true, 
                dtick: 10,
                tickmode: 'array',
                tickvals: [-100, -90, -80, -70, -60, -50, -40, -30],
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                autorange: false, // 고정 범위 사용
                fixedrange: true // 줌 비활성화
            },
            xaxis: { 
                title: 'Total Packets', 
                showgrid: true,
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                fixedrange: true // 줌 비활성화
            },
            showlegend: false,
            plot_bgcolor: 'transparent',
            paper_bgcolor: 'transparent',
            font: {
                color: 'rgba(255, 255, 255, 0.8)'
            },
            autosize: true, // 자동 크기 조정

        }, {
            displayModeBar: false,
            responsive: true // 반응형 설정
        });

        Plotly.newPlot('rcpi-chart-area', [{
            x: [],
            y: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: '#20B2AA', width: 1 },
            marker: { color: '#20B2AA', size: 2 },
            cliponaxis: false
        }], {
            margin: { t: 15, b: 35, l: 55, r: 35 }, // margin 증가
            yaxis: { 
                range: [-100, -30], 
                title: 'RCPI (dBm)', 
                showgrid: true, 
                zeroline: true, 
                dtick: 10,
                tickmode: 'array',
                tickvals: [-100, -90, -80, -70, -60, -50, -40, -30],
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                autorange: false, // 고정 범위 사용
                fixedrange: true // 줌 비활성화
            },
            xaxis: { 
                title: 'Total Packets', 
                showgrid: true,
                gridcolor: 'rgba(255, 255, 255, 0.1)',
                tickfont: { color: 'rgba(255, 255, 255, 0.8)', size: 10 },
                titlefont: { color: 'rgba(255, 255, 255, 0.9)', size: 12 },
                fixedrange: true // 줌 비활성화
            },
            showlegend: false,
            plot_bgcolor: 'transparent',
            paper_bgcolor: 'transparent',
            font: {
                color: 'rgba(255, 255, 255, 0.8)'
            },
            autosize: true, // 자동 크기 조정

        }, {
            displayModeBar: false,
            responsive: true // 반응형 설정
        });

        // 1. 통신쌍별 PRR 저장용 Map 추가 (전역)
        let communicationPairPRR = new Map();

        // PRR 등급별 색상 반환 함수 (전역)
        function getPrrGrade(value) {
            if (value >= 99.0) return { grade: 'A+', color: '#00FF00', icon: '🟢' };
            if (value >= 97.0) return { grade: 'A', color: '#90EE90', icon: '🟢' };
            if (value >= 95.0) return { grade: 'B', color: '#FFFF00', icon: '🟡' };
            if (value >= 93.0) return { grade: 'C', color: '#FFA500', icon: '🟠' };
            return { grade: 'D', color: '#FF0000', icon: '🔴' };
        }

        // OBU TX 센서 패널 업데이트 함수 내부에 아래 코드 추가
        // CAN 상세 패널 높이 동기화
        setTimeout(() => {
            const txPanel = document.getElementById('obu-tx-sensor');
            const canPanel = document.getElementById('obu-tx-can-detail');
            if (txPanel && canPanel) {
                canPanel.style.height = txPanel.offsetHeight + 'px';
            }
        }, 30);

        // CAN 상세 패널 높이 동기화 (버튼 영역까지 포함)
        setTimeout(() => {
            const txPanel = document.getElementById('obu-tx-sensor');
            const canPanel = document.getElementById('obu-tx-can-detail');
            const txControls = txPanel ? txPanel.querySelector('.sensor-controls') : null;
            if (txPanel && canPanel) {
                let totalHeight = txPanel.offsetHeight;
                if (txControls) {
                    totalHeight += txControls.offsetHeight;
                }
                canPanel.style.height = totalHeight + 'px';
            }
        }, 30);

        // CAN 상세 패널 높이 동기화 함수 정의
        function syncCanPanelHeight() {
            const txPanel = document.getElementById('obu-tx-sensor');
            const canPanel = document.getElementById('obu-tx-can-detail');
            if (txPanel && canPanel && canPanel.style.display !== 'none') {
                canPanel.style.height = txPanel.offsetHeight + 'px';
            }
        }

        // // OBU TX 센서 패널 업데이트 함수 내부에서 CAN 패널 토글 버튼 이벤트에 동기화 함수 연결
        // if (sensorControls && !document.getElementById('obu-tx-can-toggle-btn')) {
        //     const canToggleBtn = document.createElement('button');
        //     canToggleBtn.id = 'obu-tx-can-toggle-btn';
        //     canToggleBtn.className = 'sensor-control-button can-more-btn';
        //     canToggleBtn.textContent = 'CAN 값 더보기';
        //     canToggleBtn.style.cursor = 'pointer';
        //     sensorControls.appendChild(canToggleBtn);

        //     // 오른쪽 확장 패널 생성
        //     const canDetailDiv = document.createElement('div');
        //     canDetailDiv.id = 'obu-tx-can-detail';
        //     canDetailDiv.className = 'can-detail-panel';
        //     canDetailDiv.style.display = 'none';
        //     canDetailDiv.innerHTML = `
        //       <div class="can-detail-header">
        //         <span>CAN 상세정보</span>
        //       </div>
        //       <table class="can-detail-table">
        //         <tr><th>조향각(Steer_Cmd)</th><td id="obu-tx-steer">-</td></tr>
        //         <tr><th>가감속(Accel_Dec_Cmd)</th><td id="obu-tx-accel">-</td></tr>
        //         <tr><th>EPS_En</th><td id="obu-tx-eps-en">-</td></tr>
        //         <tr><th>Override_Ignore</th><td id="obu-tx-override">-</td></tr>
        //         <tr><th>EPS_Speed</th><td id="obu-tx-eps-speed">-</td></tr>
        //         <tr><th>ACC_En</th><td id="obu-tx-acc-en">-</td></tr>
        //         <tr><th>AEB_En</th><td id="obu-tx-aeb-en">-</td></tr>
        //         <tr><th>AEB_decel_value</th><td id="obu-tx-aeb-decel">-</td></tr>
        //         <tr><th>Alive_Cnt</th><td id="obu-tx-alive">-</td></tr>
        //         <tr><th>차속</th><td id="obu-tx-speed2">-</td></tr>
        //         <tr><th>브레이크 압력</th><td id="obu-tx-brake">-</td></tr>
        //         <tr><th>횡가속</th><td id="obu-tx-latacc">-</td></tr>
        //         <tr><th>요레이트</th><td id="obu-tx-yawrate">-</td></tr>
        //         <tr><th>조향각 센서</th><td id="obu-tx-steering-angle">-</td></tr>
        //         <tr><th>조향 토크(운전자)</th><td id="obu-tx-steering-drv-tq">-</td></tr>
        //         <tr><th>조향 토크(출력)</th><td id="obu-tx-steering-out-tq">-</td></tr>
        //         <tr><th>EPS Alive Count</th><td id="obu-tx-eps-alive-cnt">-</td></tr>
        //         <tr><th>ACC 상태</th><td id="obu-tx-acc-en-status">-</td></tr>
        //         <tr><th>ACC 제어보드 상태</th><td id="obu-tx-acc-ctrl-bd-status">-</td></tr>
        //         <tr><th>ACC 오류</th><td id="obu-tx-acc-err">-</td></tr>
        //         <tr><th>ACC 사용자 CAN 오류</th><td id="obu-tx-acc-user-can-err">-</td></tr>
        //         <tr><th>종가속</th><td id="obu-tx-long-accel">-</td></tr>
        //         <tr><th>우회전 신호</th><td id="obu-tx-turn-right-en">-</td></tr>
        //         <tr><th>위험신호</th><td id="obu-tx-hazard-en">-</td></tr>
        //         <tr><th>좌회전 신호</th><td id="obu-tx-turn-left-en">-</td></tr>
        //         <tr><th>ACC Alive Count</th><td id="obu-tx-acc-alive-cnt">-</td></tr>
        //         <tr><th>가속페달 위치</th><td id="obu-tx-acc-pedal-pos">-</td></tr>
        //         <tr><th>조향각 변화율</th><td id="obu-tx-steering-angle-rt">-</td></tr>
        //         <tr><th>브레이크 작동 신호</th><td id="obu-tx-brake-act-signal">-</td></tr>
        //       </table>
        //     `;
        //     // 센서패널 바로 뒤에 insert
        //     document.getElementById('obu-tx-sensor').after(canDetailDiv);

        //     canToggleBtn.onclick = function() {
        //         const isOpen = canDetailDiv.style.display === 'flex';
        //         if (!isOpen) {
        //             canDetailDiv.style.display = 'flex';
        //             canToggleBtn.classList.add('active');
        //             setTimeout(syncCanPanelHeight, 100);
        //         } else {
        //             canDetailDiv.style.display = 'none';
        //             canToggleBtn.classList.remove('active');
        //         }
        //     };
        // }
        // // 센서 패널 업데이트 후에도 동기화 시도
        // setTimeout(syncCanPanelHeight, 100);

// 페이지 언로드 시 리소스 정리
window.addEventListener('beforeunload', () => {
    clearResources();
    timers.clearAll();
    eventListeners.removeAll();
});

// DOM 캐시 초기화 (페이지 로드 시)
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initDomCache);
} else {
    initDomCache();
}






