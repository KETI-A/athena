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

window.onload = function() {
    // 모달 창 열기
    document.getElementById('modal-background').style.display = 'block';
    document.getElementById('modal').style.display = 'block';

    // 기본값 설정
    let defaultIpAddress = "10.252.110.58";
    let testMode;
    let isTxTest;
    let VisiblePathMode;
    let isVisiblePath;

    // 버튼 클릭 이벤트 처리
    document.getElementById('submit-button').onclick = function() {
        // 사용자 입력 값 가져오기
        let testType = document.getElementById('testType').value.toLowerCase();
        let ipAddress = document.getElementById('ipAddress').value || defaultIpAddress;
        let visiblePath = document.getElementById('visiblePath').value.toLowerCase();

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
        alert(`현재 선택된 테스트 모드: ${testMode}\n입력된 IP 주소: ${ipAddress}\nVisible Path ${VisiblePathMode}`);

        // 버튼이 제대로 표시되도록 모달 창이 닫힌 후 버튼을 다시 표시
        document.getElementById('autoTrackButton').style.display = 'block';
        document.getElementById('projectionButton').style.display = 'block';
        document.getElementById('connectedStatusButton').style.display = 'block';
        document.getElementById('workZoneButton').style.display = 'block';
        document.getElementById('mrsuButton').style.display = 'block';
        document.getElementById('visiblePathButton').style.display = 'block';
        document.getElementById('CB1').style.display = 'block';
        document.getElementById('CB2').style.display = 'block';
        document.getElementById('CB3').style.display = 'block';
        document.getElementById('CB4').style.display = 'block';
        document.getElementById('CB5').style.display = 'block';
        document.getElementById('CB6').style.display = 'block';

        main(isTxTest, ipAddress);
    };

    function main(isTxTest, ipAddress) {
        // KETI Pangyo
        const cKetiPangyoLatitude = 37.4064;
        const cKetiPangyolongitude = 127.1021;

        // RSU Location
        const cPangyoRsuLatitude16 = 37.409221;
        const cPangyoRsuLongitude16 = 127.099505;

        const cPangyoRsuLatitude17 = 37.406536;
        const cPangyoRsuLongitude17 = 127.100765;

        const cPangyoRsuLatitude18 = 37.405254;
        const cPangyoRsuLongitude18 = 127.103609;

        const cPangyoRsuLatitude5 = 37.410922;
        const cPangyoRsuLongitude5 = 127.094732;

        const cPangyoRsuLatitude31 = 37.411751;
        const cPangyoRsuLongitude31 = 127.095019;

        var vehicleLatitude0 = 37.406380;
        var vehicleLongitude0 = 127.102701;

        var vehicleLatitude1 = 37.406402;
        var vehicleLongitude1 = 127.102532;

        let s_nRxLatitude, s_nRxLongitude, s_unRxVehicleHeading, s_unRxVehicleSpeed;
        let s_nTxLatitude, s_nTxLongitude, s_unTxVehicleHeading, s_unTxVehicleSpeed;
        let s_unPdr, s_ulLatencyL1, s_ulTotalPacketCnt, s_unSeqNum;

        let s_unTempTxCnt = 0;
        let isPathPlan = false;
        let isCvLineEnabled = false;
        let isWorkZoneEnabled = false;
        let isMrsuEnabled = false;
        let isCentering = false;
        let isCB1 = false;
        let isCB2 = false;
        let isCB3 = false;
        let isCB4 = false;
        let isCB5 = false;
        let isCB6 = false;

        let workZoneMarker = new mapboxgl.Marker({element: createWorkZoneMarker('https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/work-zone.png')});
        let mrsuMarker = new mapboxgl.Marker({element: createWorkZoneMarker('https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/m-rsu-front.png')});

        mapboxgl.accessToken = 'pk.eyJ1IjoieWVzYm1hbiIsImEiOiJjbHoxNHVydHQyNzBzMmpzMHNobGUxNnZ6In0.bAFH10On30d_Cj-zTMi53Q';
        const map = new mapboxgl.Map({
            container: 'map',
            style: 'mapbox://styles/yesbman/clyzkeh8900dr01pxdqow8awk',
            projection: 'globe',
            zoom: 19,
            center: [cKetiPangyolongitude, cKetiPangyoLatitude]
        });

        map.addControl(new mapboxgl.NavigationControl());

        map.on('style.load', () => {
            map.setFog({});

            document.getElementById('autoTrackButton').addEventListener('click', function() {
                isCentering = !isCentering;
                if (isCentering) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }

                map.setCenter([s_nRxLongitude, s_nRxLatitude]);
            });
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

        document.getElementById('projectionButton').addEventListener('click', function() {
            isPathPlan = !isPathPlan;
            if (isPathPlan) {
                this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                this.style.color = 'white';
            } else {
                this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                this.style.color = 'white';
            }

            map.setCenter([s_nRxLongitude, s_nRxLatitude]);
        });

        document.getElementById('connectedStatusButton').addEventListener('click', function() {
            isCvLineEnabled = !isCvLineEnabled;
            if (isCvLineEnabled) {
                this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                this.style.color = 'white';
            } else {
                this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                this.style.color = 'white';
            }


            if (isCvLineEnabled) {
                if (!map.getLayer('lineLayer')) {
                    map.addSource('line', {
                        'type': 'geojson',
                        'data': {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'LineString',
                                'coordinates': [[vehicleLongitude0, vehicleLatitude0], [vehicleLongitude1, vehicleLatitude1]]
                            }
                        }
                    });

                    map.addLayer({
                        'id': 'lineLayer',
                        'type': 'line',
                        'source': 'line',
                        'layout': {},
                        'paint': {
                            'line-color': [
                                'interpolate',
                                ['linear'],
                                ['line-progress'],
                                0, '#00FFFF',
                                1, '#008B8B'
                            ],
                            'line-width': 1.5
                        }
                    });

                    map.addLayer({
                        'id': 'lineLabelLayer',
                        'type': 'symbol',
                        'source': 'line',
                        'layout': {
                            'symbol-placement': 'line',
                            'text-field': 'Connected V2X',
                            'text-font': ['Open Sans Regular', 'Arial Unicode MS Regular'],
                            'text-size': 12,
                            'text-anchor': 'center'
                        },
                        'paint': {
                            'text-color': '#000000',
                            'text-halo-color': '#FFFFFF',
                            'text-halo-width': 2
                        }
                    });
                } else {
                    map.getSource('line').setData({
                        'type': 'Feature',
                        'geometry': {
                            'type': 'LineString',
                            'coordinates': [[vehicleLongitude0, vehicleLatitude0], [vehicleLongitude1, vehicleLatitude1]]
                        }
                    });
                }
            } else {
                // 연결 선 제거
                if (map.getLayer('lineLayer')) {
                    map.removeLayer('lineLayer');
                }
                if (map.getLayer('lineLabelLayer')) {
                    map.removeLayer('lineLabelLayer');
                }
                if (map.getSource('line')) {
                    map.removeSource('line');
                }
            }
        });

        document.getElementById('workZoneButton').addEventListener('click', function() {
            isWorkZoneEnabled = !isWorkZoneEnabled;
            if (isWorkZoneEnabled) {
                this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                this.style.color = 'white';
            } else {
                this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                this.style.color = 'white';
            }
            toggleWorkZone();
        });

        function toggleWorkZone()
        {
            const WorkZoneCoordinate = [127.4401043, 36.7298408];
            if (isWorkZoneEnabled)
            {
                if (workZoneMarker === null)
                {
                    workZoneMarker = new mapboxgl.Marker({element: createWorkZoneMarker('https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/work-zone.png')})
                    .setLngLat(WorkZoneCoordinate)
                    .addTo(map);
                }
            }
            else
            {
                if (workZoneMarker !== null)
                {
                    workZoneMarker.remove();
                    workZoneMarker = null;
                }
            }
        }

        function createWorkZoneMarker(imageUrl)
        {
            const workzonecontainer = document.createElement('div');
            workzonecontainer.style.display = 'flex';
            workzonecontainer.style.flexDirection = 'column';
            workzonecontainer.style.alignItems = 'center';

            const img = document.createElement('div');
            img.style.backgroundImage = `url(${imageUrl})`;
            img.style.width = '80px';
            img.style.height = '80px';
            img.style.backgroundSize = 'contain';
            img.style.backgroundRepeat = 'no-repeat';

            const label = document.createElement('div');
            label.textContent = "공사중";
            label.style.color = 'white';
            label.style.textAlign = 'center';
            label.style.fontWeight = 'bold';
            label.style.backgroundColor = 'rgba(255, 0, 0, 0.97)';
            label.style.padding = '5px 10px';
            label.style.borderRadius = '10px';
            label.style.boxShadow = '0 0 15px #00ccff, 0 0 30px #00ccff, 0 0 45px #00ccff';
            label.style.textShadow = '0 0 10px #00ccff, 0 0 20px #00ccff, 0 0 30px #00ccff';
            label.style.boxShadow = '0px 4px 8px rgba(0, 0, 0, 0.3)';
            label.style.width = 'auto';
            label.style.display = 'inline-block';
            label.style.fontSize = '18px';

            workzonecontainer.appendChild(img);
            workzonecontainer.appendChild(label);

            return workzonecontainer;
        }

        document.getElementById('mrsuButton').addEventListener('click', function() {
            isMrsuEnabled = !isMrsuEnabled;
            if (isMrsuEnabled) {
                this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                this.style.color = 'white';
            } else {
                this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                this.style.color = 'white';
            }

            toggleMrsu();
        });

        document.getElementById('visiblePathButton').addEventListener('click', function() {
            isVisiblePath = !isVisiblePath;
            if (isVisiblePath) {
                this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                this.style.color = 'white';
            } else {
                this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                this.style.color = 'white';
            }

        });

        function toggleMrsu()
        {
            const MRsuCoordinate = [127.440227, 36.730164];

            if (isMrsuEnabled)
            {
                if (mrsuMarker === null)
                {
                    mrsuMarker = new mapboxgl.Marker({element: createMrsuMarker('https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/m-rsu-front.png')})
                    .setLngLat(MRsuCoordinate)
                    .addTo(map);
                }
            }
            else
            {
                if (mrsuMarker !== null)
                {
                    mrsuMarker.remove();
                    mrsuMarker = null;
                }
            }
        }

        function createMrsuMarker(imageUrl)
        {
            const mrsucontainer = document.createElement('div');
            mrsucontainer.style.display = 'flex';
            mrsucontainer.style.flexDirection = 'column';
            mrsucontainer.style.alignItems = 'center';
            mrsucontainer.style.width = '200px';

            const img = document.createElement('div');
            img.style.backgroundImage = `url(${imageUrl})`;
            img.style.width = '225px';
            img.style.height = '170px';
            img.style.backgroundSize = 'contain';
            img.style.backgroundRepeat = 'no-repeat';

            const label = document.createElement('div');
            label.textContent = "RSU";
            label.style.color = 'white';
            label.style.textAlign = 'center';
            label.style.fontWeight = 'bold';
            label.style.backgroundColor = 'rgba(0, 204, 255, 0.8)';
            label.style.padding = '5px 10px';
            label.style.borderRadius = '10px';
            label.style.boxShadow = '0 0 15px #00ccff, 0 0 30px #00ccff, 0 0 45px #00ccff';
            label.style.textShadow = '0 0 10px #00ccff, 0 0 20px #00ccff, 0 0 30px #00ccff';
            label.style.boxShadow = '0px 4px 8px rgba(0, 0, 0, 0.3)';
            label.style.width = 'auto';
            label.style.display = 'inline-block';
            label.style.fontSize = '18px';
            label.style.marginLeft = '-20px';

            mrsucontainer.appendChild(img);
            mrsucontainer.appendChild(label);

            return mrsucontainer;
        }

        map.on('style.load', () => {
            document.getElementById('CB1').addEventListener('click', function() {
                isCB1 = !isCB1;
                if (isCB1) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }
            });
        });

        const classBCoordinates = [
            [127.439641, 36.730080],
            [127.439703, 36.730085], //첫번째 노란점
            [127.439820, 36.730091],
            [127.439885, 36.730050], //두번째 노란점
            [127.439991, 36.729972], //세번째 노란점
            [127.440131, 36.729958],
            [127.440254, 36.730017], //네번째
            [127.440304, 36.730084], //다섯번째
            [127.440352, 36.730148],
            [127.440451, 36.730166] //마지막
        ];

        function interpolateCatmullRom(points, numPointsBetween) {
            let interpolatedPoints = [];

            function interpolate(p0, p1, p2, p3, t) {
                const t2 = t * t;
                const t3 = t2 * t;
                const out = [
                    0.5 * (2 * p1[0] + (-p0[0] + p2[0]) * t + (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t2 + (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t3),
                    0.5 * (2 * p1[1] + (-p0[1] + p2[1]) * t + (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t2 + (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t3)
                ];
                return out;
            }

            for (let i = 1; i < points.length - 2; i++) {
                const p0 = points[i - 1];
                const p1 = points[i];
                const p2 = points[i + 1];
                const p3 = points[i + 2];

                interpolatedPoints.push(p1);
                for (let t = 0; t < numPointsBetween; t++) {
                    const tNorm = t / numPointsBetween;
                    interpolatedPoints.push(interpolate(p0, p1, p2, p3, tNorm));
                }
            }
            interpolatedPoints.push(points[points.length - 2]);
            interpolatedPoints.push(points[points.length - 1]);

            return interpolatedPoints;
        }

        const smoothPath = interpolateCatmullRom(classBCoordinates, 100);

        map.on('style.load', function()
        {
            map.loadImage('https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/arrowB.png', function(error, image)
            {
                if (error)
                {
                    console.error('fail load image', error);
                    return;
                }

            map.addImage('arrowB-icon', image);

            document.getElementById('CB2').addEventListener('click', function()
            {
                isCB2 = !isCB2;
                if (isCB2) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }

                if (map.getLayer('classBPath'))
                {
                    map.setLayoutProperty('classBPath', 'visibility', isCB2 ? 'visible' : 'none');
                    map.setLayoutProperty('classBArrows', 'visibility', isCB2 ? 'visible' : 'none');
                }
                else
                {
                    map.addSource('classBPath', {
                        'type': 'geojson',
                        'data': {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'LineString',
                                'coordinates': smoothPath
                            }
                        }
                    });

                    map.addLayer({
                        'id': 'classBPath',
                        'type': 'line',
                        'source': 'classBPath',
                        'layout': {
                            'line-join': 'round',
                            'line-cap': 'round',
                            'visibility': 'visible'
                        },
                        'paint': {
                            'line-color': 'rgba(0, 150, 255, 0.8)',
                            'line-width': 20,
                            'line-blur': 0.5
                        }
                    });

                    const arrowCoordinates = [
                        { coord: [127.439703, 36.730085], rotate: 90},
                        { coord: [127.439885, 36.730050], rotate: 140},
                        { coord: [127.439991, 36.729972], rotate: 110},
                        { coord: [127.440254, 36.730017], rotate: 45},
                        { coord: [127.440304, 36.730084], rotate: 30},
                        { coord: [127.440451, 36.730166], rotate: 85}
                    ];

                    const arrowFeatures = arrowCoordinates.map(arrow => {
                        return {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': arrow.coord
                            },
                            'properties': {
                                'rotate': arrow.rotate
                            }
                        };
                    });

                    map.addSource('classBArrows', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': arrowFeatures
                        }
                    });

                    map.addLayer({
                        'id': 'classBArrows',
                        'type': 'symbol',
                        'source': 'classBArrows',
                        'layout': {
                            'icon-image': 'arrowB-icon',
                            'icon-size': 0.05,
                            'icon-rotate': ['get', 'rotate'],
                            'icon-allow-overlap': true,
                            'visibility': 'visible'
                        }
                    });
                }
                });
            });
        });

        map.on('style.load', function()
        {
            map.loadImage('https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/arrowG.png', function(error, image)
            {
                if (error)
                {
                    console.error('fail load image', error);
                    return;
                }

            map.addImage('arrowG-icon', image);

            document.getElementById('CB3').addEventListener('click', function()
            {
                isCB3 = !isCB3;
                if (isCB3) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }

                if (map.getLayer('CB3Path'))
                {
                    map.setLayoutProperty('CB3Path', 'visibility', isCB3 ? 'visible' : 'none');
                    map.setLayoutProperty('CB3Arrows', 'visibility', isCB3 ? 'visible' : 'none');
                }
                else
                {
                    const CB3Coordinates = [
                        { coord: [127.440170, 36.729793]},
                        { coord: [127.440157, 36.729847], rotate: 0},
                        { coord: [127.440181, 36.729961]},
                        { coord: [127.440254, 36.730017], rotate: 45},
                        { coord: [127.440304, 36.730084], rotate: 30},
                        { coord: [127.440350, 36.730151]},
                        { coord: [127.440451, 36.730166], rotate: 85}
                    ];

                    const CB3route = CB3Coordinates.map(point => point.coord);
                    const smoothCB3route = interpolateCatmullRom(CB3route, 100);

                    map.addSource('CB3Path', {
                        'type': 'geojson',
                        'data': {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'LineString',
                                'coordinates': smoothCB3route
                            }
                        }
                    });

                    map.addLayer({
                        'id': 'CB3Path',
                        'type': 'line',
                        'source': 'CB3Path',
                        'layout': {
                            'line-join': 'round',
                            'line-cap': 'round',
                            'visibility': 'visible'
                        },
                        'paint': {
                            'line-color': 'rgba(50, 205, 50, 0.7)',
                            'line-width': 20,
                            'line-blur': 1,
                            'line-opacity': 0.8
                        }
                    });

                    const arrowFeatures = CB3Coordinates
                        .filter(arrow => arrow.rotate !== undefined)
                        .map(arrow => {
                        return {
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': arrow.coord
                            },
                            'properties': {
                                'rotate': arrow.rotate
                            }
                        };
                    });

                    map.addSource('CB3Arrows', {
                        'type': 'geojson',
                        'data': {
                            'type': 'FeatureCollection',
                            'features': arrowFeatures
                        }
                    });

                    map.addLayer({
                        'id': 'CB3Arrows',
                        'type': 'symbol',
                        'source': 'CB3Arrows',
                        'layout': {
                            'icon-image': 'arrowG-icon',
                            'icon-size': 0.05,
                            'icon-rotate': ['get', 'rotate'],
                            'icon-allow-overlap': true,
                            'visibility': 'visible'
                        }
                    });
                }
            });
        });
        });

        map.on('style.load', () => {
            document.getElementById('CB4').addEventListener('click', function() {
                isCB4 = !isCB4;
                if (isCB4) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }
            });
        });

        map.on('style.load', () => {
            document.getElementById('CB5').addEventListener('click', function() {
                isCB5 = !isCB5;
                if (isCB5) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }
            });
        });

        map.on('style.load', () => {
            document.getElementById('CB6').addEventListener('click', function() {
                isCB6 = !isCB6;
                if (isCB6) {
                    this.style.backgroundColor = 'rgba(0, 122, 255, 0.9)';
                    this.style.color = 'white';
                } else {
                    this.style.backgroundColor = 'rgba(0, 0, 0, 0.7)';
                    this.style.color = 'white';
                }
            });
        });

        if ('WebSocket' in window) {
            let ws = new WebSocket(`ws://${ipAddress}:3001/websocket`);
            ws.onopen = () => {
                console.log('WebSocket connection established');
                ws.send('Client connected');
            }

            function reverseHeading(heading) {
                // heading 값을 180도 회전시키고 좌우를 반대로 변환
                let reversedHeading = (360 - ((parseInt(heading) + 180) % 360)) % 360;
                return reversedHeading;
            }

            if(isTxTest) {
                ws.onmessage = (message) => {
                    let data = message.data.split(',');
                    s_nRxLatitude = data[23];
                    s_nRxLongitude = data[24];
                    s_unRxVehicleSpeed = data[28];
                    s_unRxVehicleHeading = reverseHeading(data[29]);
                    s_nTxLatitude = vehicleLatitude1;
                    s_nTxLongitude = vehicleLongitude1;
                    s_unTxVehicleHeading = 90;
                    s_unPdr = 100;
                    s_ulLatencyL1 = 500;
                    s_ulTotalPacketCnt = 1 + s_unTempTxCnt;
                    s_unSeqNum = 1 + s_unTempTxCnt;

                    s_unTempTxCnt += 1;
                }
            }
            else
            {
                ws.onmessage = (message) => {
                    let data = message.data.split(',');
                    s_nRxLatitude = data[62];
                    s_nRxLongitude = data[63];
                    s_unRxVehicleSpeed = data[55];
                    s_unRxVehicleHeading = reverseHeading(data[56]);
                    s_nTxLatitude = data[32];
                    s_nTxLongitude = data[33];
                    s_unTxVehicleSpeed = data[37];
                    s_unTxVehicleHeading = reverseHeading(data[38]);
                    s_unPdr = data[68];
                    s_ulLatencyL1 = data[43];
                    s_ulTotalPacketCnt = data[66];
                    s_unSeqNum = data[35];
                }
            }

            ws.onerror = (error) => {
                console.error('WebSocket error', error);
            }

            ws.onclose = () => {
                console.log('WebSocket connection closed');
            }
        } else {
            console.error('WebSocket is not supported by this browser');
        }

        /************************************************************/
        /* KD Tree */
        /************************************************************/
        let roadNetworkCoordinates = [];
        let tree;

        map.on('style.load', () => {
            console.log("Map style loaded successfully.");
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
                    console.log("Fetched GeoJSON Data:", geojsonData);

                    // GeoJSON 데이터가 유효한지 확인
                    if (geojsonData && geojsonData.type === 'FeatureCollection' && Array.isArray(geojsonData.features)) {
                        console.log("GeoJSON data validated. Adding to map...");

                        // GeoJSON 데이터를 Mapbox에 소스로 추가
                        map.addSource('road-network', {
                            'type': 'geojson',
                            'data': geojsonData
                        });

                        console.log("Road network source successfully added.");

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
                        console.log("KD-Tree Input Coordinate:", coord); // 좌표를 출력
                        roadNetworkCoordinates.push([coord[0], coord[1]]);
                    });
                } else if (feature.geometry.type === "MultiLineString") {
                    feature.geometry.coordinates.forEach(line => {
                        line.forEach(coord => {
                            console.log("KD-Tree Input Coordinate:", coord); // 좌표를 출력
                            roadNetworkCoordinates.push([coord[0], coord[1]]);
                        });
                    });
                }
            });

            // KD-Tree 생성 및 전역 변수로 설정
            tree = new KDTree(roadNetworkCoordinates, euclideanDistance);

            console.log("KD-Tree built successfully with", roadNetworkCoordinates.length, "points.");

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

            console.log("Total Points After Interpolation:", roadNetworkCoordinates.length);  // 보간된 좌표 로그 출력

            // KD-Tree 생성 및 전역 변수로 설정
            //tree = new KDTree(roadNetworkCoordinates, euclideanDistance);
            // KD-Tree 생성 시 거리 계산을 Haversine Formula로 변경
            tree = new KDTree(roadNetworkCoordinates, haversineDistance);

            console.log("KD-Tree built successfully with", roadNetworkCoordinates.length, "points.");

            // KD-Tree 좌표를 지도에 추가
            if (isVisiblePath) {
                addPointsToMapOfKdTree(roadNetworkCoordinates);
            }
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
            console.log("Added points to map:", points);
        }

        /************************************************************/
        /* Map */
        /************************************************************/
        map.on('load', () => {
            console.log("Map loaded successfully.");

            map.on('zoom', () => {
                map.resize();  // 확대/축소 시 강제로 지도를 리렌더링
            });

            map.on('move', () => {
                map.resize();  // 지도가 이동될 때 리렌더링
            });

            /************************************************************/
            /* CI */
            /************************************************************/
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
                            'icon-size': 0.50
                        }
                    });
                }
            );

            /************************************************************/
            /* RSU */
            /************************************************************/
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
                            'icon-size': 0.5
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
                            'icon-size': 0.5
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
                            'icon-size': 0.5
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
                    'icon-size': 0.5
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
                    'icon-size': 0.5
                }
            });

            /************************************************************/
            /* Vehicle 0 */
            /************************************************************/
            map.loadImage(
                'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq5.png',
                (error, image) => {
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
                            'icon-size': 0.2,
                            'icon-rotate': ['get', 'heading'],
                            'text-field': ['concat', 'Heading ', ['get', 'heading']],
                            'text-font': ['Open Sans Semibold', 'Arial Unicode MS Bold'],
                            'text-offset': [0, 1.25], // Adjust this value to position the text
                            'text-anchor': 'top'
                        },
                        'paint': {
                            'text-color': '#000000',
                            'text-halo-color': '#FFFFFF',
                            'text-halo-width': 2
                        }
                    });

                    fetchAndUpdate();
                    setInterval(fetchAndUpdate, 10);
                }
            );

            /************************************************************/
            /* Vehicle 1 */
            /************************************************************/
            map.loadImage(
                'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/ioniq-electric.png',
                (error, image) => {
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
                            'icon-size': 0.2,
                            'icon-rotate': ['get', 'heading'],
                            'text-field': ['concat', 'Heading ', ['get', 'heading']],
                            'text-font': ['Open Sans Semibold', 'Arial Unicode MS Bold'],
                            'text-offset': [0, 1.25], // Adjust this value to position the text
                            'text-anchor': 'top'
                        },
                        'paint': {
                            'text-color': '#000000',
                            'text-halo-color': '#FFFFFF',
                            'text-halo-width': 2
                        }
                    });

                    fetchAndUpdate();
                    setInterval(fetchAndUpdate, 10);
                }
            );

            console.log("Road network and vehicle sources added successfully.");
        });

        /************************************************************/
        /* Update Position */
        /************************************************************/
        function updateVehiclePosition(vehicleId, coordinates, heading) {
            let vehicleSource = map.getSource(`vehicle_src_${vehicleId}`);
            let snappedCoordinates = coordinates;  // 기본값을 실제 GPS 좌표로 설정

            /* KD Tree Path: KD 트리를 사용할 때 스냅된 좌표로 업데이트 */
            if (tree && isPathPlan) {
                let point = {
                    longitude: coordinates[0],
                    latitude: coordinates[1]
                };

                let nearest = tree.nearest([point.longitude, point.latitude], 1);

                if (nearest.length > 0) {
                    snappedCoordinates = nearest[0]; // 가장 가까운 점을 스냅된 좌표로 설정
                    // KD 트리를 사용하는 경우 지도 위 차량 위치 업데이트
                    if (vehicleSource) {
                        vehicleSource.setData({
                            'type': 'FeatureCollection',
                            'features': [{
                                'type': 'Feature',
                                'geometry': {
                                    'type': 'Point',
                                    'coordinates': snappedCoordinates  // KD 트리 스냅된 좌표 사용
                                },
                                'properties': {
                                    'heading': heading
                                }
                            }]
                        });
                    } else {
                        console.warn(`Vehicle source vehicle_src_${vehicleId} not found.`);
                    }

                    if(isVisiblePath) {
                    updateSnappedPath(snappedCoordinates);
                    }
                } else {
                    console.warn("No nearest point found in KD Tree.");
                }
            } else {
                // Real GPS Path: 실제 GPS 좌표로 지도 위 차량 위치 업데이트
                if (vehicleSource) {
                    vehicleSource.setData({
                        'type': 'FeatureCollection',
                        'features': [{
                            'type': 'Feature',
                            'geometry': {
                                'type': 'Point',
                                'coordinates': coordinates  // 실제 GPS 좌표 사용
                            },
                            'properties': {
                                'heading': heading
                            }
                        }]
                    });
                } else {
                    console.warn(`Vehicle source vehicle_src_${vehicleId} not found.`);
                }
            }

            // 상태 업데이트 (스냅된 좌표 또는 실제 좌표로 업데이트)
            if (isPathPlan) {
                // KD 트리의 스냅된 좌표로 상태 업데이트
                if (vehicleId === 0) {
                    vehicleLongitude0 = snappedCoordinates[0];
                    vehicleLatitude0 = snappedCoordinates[1];
                } else if (vehicleId === 1) {
                    vehicleLongitude1 = snappedCoordinates[0];
                    vehicleLatitude1 = snappedCoordinates[1];
                }
            } else {
                // 실제 GPS 좌표로 상태 업데이트
                if (vehicleId === 0) {
                    vehicleLongitude0 = coordinates[0];
                    vehicleLatitude0 = coordinates[1];
                } else if (vehicleId === 1) {
                    vehicleLongitude1 = coordinates[0];
                    vehicleLatitude1 = coordinates[1];
                }
            }

            // 연결된 선 업데이트 (isCvLineEnabled가 true일 때만)
            if (isCvLineEnabled && map.getSource('line')) {
                map.getSource('line').setData({
                    'type': 'Feature',
                    'geometry': {
                        'type': 'LineString',
                        'coordinates': [[vehicleLongitude0, vehicleLatitude0], [vehicleLongitude1, vehicleLatitude1]]
                    }
                });
            }

            if (isCentering && vehicleId === 0) {
                map.setCenter(snappedCoordinates); // 지도 중심을 스냅된 좌표로 설정
            }

            if(isVisiblePath) {
                updateGpsPath(coordinates); // 실제 GPS 좌표를 경로로 업데이트
            }
        }

        // 단일 GPS 좌표를 추가하는 함수
        function updateGpsPath(coordinate) {
            // 기존 소스가 없으면 생성
            if (!map.getSource('gps-path')) {
                map.addSource('gps-path', {
                    'type': 'geojson',
                    'data': {
                        'type': 'FeatureCollection',
                        'features': []
                    }
                });
            }

            // 현재 소스 데이터 가져오기
            const source = map.getSource('gps-path');
            const data = source._data || { 'type': 'FeatureCollection', 'features': [] };

            // 새로운 점 추가
            data.features.push({
                'type': 'Feature',
                'geometry': {
                    'type': 'Point',
                    'coordinates': coordinate
                }
            });

            // 소스 데이터 업데이트
            source.setData(data);

            // 레이어가 없으면 추가
            if (!map.getLayer('gps-path-layer')) {
                map.addLayer({
                    'id': 'gps-path-layer',
                    'type': 'circle',
                    'source': 'gps-path',
                    'paint': {
                        'circle-radius': 3,
                        'circle-color': '#FF0000',
                    }
                });
            }
        }

        /************************************************************/
        /* Update Snapped (KD Tree) Path with Blue Dots */
        /************************************************************/
        function updateSnappedPath(coordinate) {
            // 기존 소스가 없으면 생성
            if (!map.getSource('snapped-path')) {
                map.addSource('snapped-path', {
                    'type': 'geojson',
                    'data': {
                        'type': 'FeatureCollection',
                        'features': []
                    }
                });
            }

            // 현재 소스 데이터 가져오기
            const source = map.getSource('snapped-path');
            const data = source._data || { 'type': 'FeatureCollection', 'features': [] };

            // 새로운 스냅된 점 추가
            data.features.push({
                'type': 'Feature',
                'geometry': {
                    'type': 'Point',
                    'coordinates': coordinate
                }
            });

            // 소스 데이터 업데이트
            source.setData(data);

            // 레이어가 없으면 추가
            if (!map.getLayer('snapped-path-layer')) {
                map.addLayer({
                    'id': 'snapped-path-layer',
                    'type': 'circle',
                    'source': 'snapped-path',
                    'paint': {
                        'circle-radius': 3,
                        'circle-color': '#0000FF',  // 파란색 점
                    }
                });
            }
        }

        function updateTrafficLightBasedOnHeading(heading) {
            const trafficLightImage = document.getElementById('traffic-light-image');
            const trafficLightStatus = document.getElementById('traffic-light-status');

            if (heading >= 0 && heading <= 90) {
                trafficLightImage.src = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/green-light.png';  // 초록 불로 변경
                trafficLightStatus.innerHTML = 'Green Light<br>(GO)';       // 텍스트 업데이트
                trafficLightStatus.style.color = 'green';
            } else if (heading >= 91 && heading <= 180) {
                trafficLightImage.src = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/yellow-light.png';  // 주황 불로 변경
                trafficLightStatus.innerHTML = 'Yellow Light<br>(SLOW)';      // 텍스트 업데이트
                trafficLightStatus.style.color = 'orange';
            } else if (heading >= 181 && heading <= 359) {
                trafficLightImage.src = 'https://raw.githubusercontent.com/KETI-A/athena/main/src/apps/html/images/red-light.png';  // 빨간 불로 변경
                trafficLightStatus.innerHTML = 'RED Light<br>(STOP)';         // 텍스트 업데이트
                trafficLightStatus.style.color = 'red';
            }
        }

        function updateHeadingInfo(heading) {
            const headingText = document.getElementById('heading-text');
            headingText.innerText = `${heading}°`;
        }

        function updateSpeedInfo(speed) {
            const speedValue = document.getElementById('speed-value');
            speedValue.innerText = speed;
        }


        function fetchAndUpdate() {
            if (!tree) {
                console.warn("KD-Tree is not built yet. Waiting...");
                return;
            }

            const latitude0 = parseFloat(s_nRxLatitude);
            const longitude0 = parseFloat(s_nRxLongitude);
            const heading0 = parseFloat(s_unRxVehicleHeading);
            const speed0 = parseFloat(s_unRxVehicleSpeed);
            const latitude1 = parseFloat(s_nTxLatitude);
            const longitude1 = parseFloat(s_nTxLongitude);
            const heading1 = parseFloat(s_unTxVehicleHeading);

            if (!isNaN(latitude0) && !isNaN(longitude0)) {
                updateVehiclePosition(0, [longitude0, latitude0], heading0);
                updateTrafficLightBasedOnHeading(heading0);  // 신호등 업데이트
                updateHeadingInfo(heading0);
                updateSpeedInfo(speed0);
            }

            if (!isNaN(latitude1) && !isNaN(longitude1)) {
                updateVehiclePosition(1, [longitude1, latitude1], heading1);
            }
        }

        /************************************************************/
        /* Graph */
        /************************************************************/
        function fetchAndUpdateGraph() {
            const unPdr = parseFloat(s_unPdr);
            const ulLatencyL1 = parseFloat(s_ulLatencyL1);
            const ulTotalPacketCnt = parseFloat(s_ulTotalPacketCnt);

            if (!isNaN(unPdr) && !isNaN(ulTotalPacketCnt)) {
                updateGraph1(ulTotalPacketCnt, unPdr);
            } else {
                console.error('Invalid data points for Graph1.');
            }

            if (!isNaN(ulLatencyL1) && !isNaN(ulTotalPacketCnt)) {
                updateGraph2(ulTotalPacketCnt, ulLatencyL1 / 100);
            } else {
                console.error('Invalid data points for Graph2.');
            }
        }

        fetchAndUpdateGraph();
        setInterval(fetchAndUpdateGraph, 100);

        function updateGraph1(xValue, unPdrValue) {
            if (!Array.isArray(xValue)) {
                xValue = [xValue];
            }
            if (!Array.isArray(unPdrValue)) {
                unPdrValue = [unPdrValue];
            }

            if (!isNaN(xValue[0]) && !isNaN(unPdrValue[0])) {
                Plotly.extendTraces('graph1', {
                    x: [xValue],
                    y: [unPdrValue]
                }, [0]);

                // TotalPacketCount 텍스트 추가
                let totalPacketCount = xValue[0];
                let middleYValue = (80 + 100) / 2;

                Plotly.relayout('graph1', {
                    yaxis: {
                        range: [80, 100],
                        title: 'PDR (Packet Delivery Rate) (%)',
                        dtick: 1,
                        tickfont: {
                            size: 10  // y축 숫자 글씨 크기 줄이기
                        }
                    },
                    xaxis: {
                        title: 'The Total Received Rx Packets',
                        tickfont: {
                            size: 10  // x축 숫자 글씨 크기 줄이기
                        }
                    },
                    annotations: [
                        {
                            x: totalPacketCount,
                            y: middleYValue,
                            xref: 'x',
                            yref: 'y',
                            text: `Received Total Tx Packets: ${s_unSeqNum}<br>Received Total Rx Packets: ${totalPacketCount}`,
                            showarrow: false,
                            font: {
                                family: 'Arial, sans-serif',
                                size: 16,
                                color: 'black',
                                weight: 'bold'
                            },
                            align: 'center',
                            bordercolor: 'black',
                            borderwidth: 1,
                            borderpad: 4,
                            bgcolor: '#ffffff',
                            opacity: 0.8
                        }
                    ]
                });

                document.getElementById('pdr-value').innerText = `PDR(Packet Delivery Rate) ${unPdrValue[0]}%`;
            } else {
                console.error('Invalid data points for Graph1.');
            }
        }

        let latencyData = [];

        function updateGraph2(xValue, ulLatencyValue) {
            if (!Array.isArray(xValue)) {
                xValue = [xValue];
            }
            if (!Array.isArray(ulLatencyValue)) {
                ulLatencyValue = [ulLatencyValue];
            }

            if (!isNaN(xValue[0]) && !isNaN(ulLatencyValue[0])) {
                latencyData.push({x: xValue[0], y: ulLatencyValue[0]});

                Plotly.update('graph2', {
                    x: [latencyData.map(point => point.x)],
                    y: [latencyData.map(point => point.y)]
                }, [0]);

                let avgLatency = latencyData.reduce((sum, point) => sum + point.y, 0) / latencyData.length;

                Plotly.relayout('graph2', {
                    yaxis: {
                        range: [0, 15],
                        title: 'Latency (ms)',
                        dtick: 1,
                        tickfont: {
                            size: 10  // y축 숫자 글씨 크기 줄이기
                        }
                    },
                    xaxis: {
                        title: 'The Total Received Rx Packets',
                        tickfont: {
                            size: 10  // x축 숫자 글씨 크기 줄이기
                        }
                    },
                    shapes: [
                        {
                            type: 'line',
                            x0: latencyData[0].x, x1: latencyData[latencyData.length - 1].x,
                            y0: avgLatency, y1: avgLatency,
                            line: {
                                color: 'red',
                                width: 2,
                                dash: 'dash'
                            }
                        }
                    ],
                    annotations: [
                        {
                            x: latencyData[latencyData.length - 1].x,
                            y: avgLatency,
                            xref: 'x',
                            yref: 'y',
                            text: `Avg: ${avgLatency.toFixed(2)} ms`,
                            showarrow: false,
                            font: {
                                family: 'Arial, sans-serif',
                                size: 16,
                                color: 'red',
                            },
                            align: 'right',
                            xanchor: 'left',
                            yanchor: 'bottom',
                            bordercolor: 'red',
                            borderwidth: 2,
                            borderpad: 4,
                            bgcolor: '#ffffff',
                            opacity: 0.8
                        }
                    ]
                });

                document.getElementById('latency-value').innerText = `Latency(Air to Air) ${ulLatencyValue[0]}ms, Avg: ${avgLatency.toFixed(2)}ms`;
            } else {
                console.error('Invalid data points for Graph2.');
            }
        }

        Plotly.newPlot('graph1', [{
            x: [],
            y: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: 'green', width: 2 },
            marker: { color: 'green', size: 6 }
        }], {
            margin: { t: 60, b: 40, l: 50, r: 30 }, // 타이틀 높이에 맞게 top margin 증가
            yaxis: { range: [80, 100], title: 'PDR (%)', showgrid: true, zeroline: true, dtick: 1 },
            xaxis: { title: 'ulTotalPacketCnt', showgrid: true },
            title: {
                text: 'Real-time PDR Monitoring',
                font: {
                    size: 20,  // 타이틀 글자 크기만 설정
                    color: 'white'
                },
                x: 0.5,  // 중앙 정렬
                xanchor: 'center',
                yanchor: 'top'
            },
            plot_bgcolor: 'rgba(0, 0, 0, 0.7)',  // 그래프 내부 배경
            paper_bgcolor: 'rgba(0, 0, 0, 0.7)', // 그래프 전체 배경
            font: {
                color: 'white'
            },
            xaxis: {
                gridcolor: 'rgba(255, 255, 255, 0.3)',
            },
            yaxis: {
                gridcolor: 'rgba(255, 255, 255, 0.3)',
            }
        });

        Plotly.newPlot('graph2', [{
            x: [],
            y: [],
            type: 'scatter',
            mode: 'lines+markers',
            line: { color: 'blue', width: 2 },
            marker: { color: 'blue', size: 6 }
        }], {
            margin: { t: 60, b: 40, l: 50, r: 30 }, // 타이틀 높이에 맞게 top margin 증가
            yaxis: { range: [0, 15], title: 'Latency (ms)', showgrid: true, zeroline: true, dtick: 1 },
            xaxis: { title: 'ulTotalPacketCnt', showgrid: true },
            title: {
                text: 'Real-time Latency Monitoring',
                font: {
                    size: 20,  // 타이틀 글자 크기만 설정
                    color: 'white'
                },
                x: 0.5,  // 중앙 정렬
                xanchor: 'center',
                yanchor: 'top'
            },
            plot_bgcolor: 'rgba(0, 0, 0, 0.7)',  // 그래프 내부 배경
            paper_bgcolor: 'rgba(0, 0, 0, 0.7)', // 그래프 전체 배경
            font: {
                color: 'white'
            },
            xaxis: {
                gridcolor: 'rgba(255, 255, 255, 0.3)',
            },
            yaxis: {
                gridcolor: 'rgba(255, 255, 255, 0.3)',
            }
        });

        const weatherApiKey = '0384422edd4701383345e4e16d05b903';

        function updateWeather() {
            const center = map.getCenter();
            const lat = center.lat;
            const lon = center.lng;

            fetch(`https://api.openweathermap.org/data/2.5/weather?lat=${lat}&lon=${lon}&units=metric&appid=${weatherApiKey}`)
                .then(response => response.json())
                .then(data => {
                    console.log(data); // API 응답 데이터를 콘솔에 출력하여 확인

                    if (data.cod === 200) {
                        const icon = data.weather[0].icon;
                        const temp = data.main.temp;
                        const humidity = data.main.humidity;
                        const location = data.name;

                        document.getElementById('weather-icon').src = `https://openweathermap.org/img/wn/${icon}.png`;
                        document.getElementById('location').textContent = `Location: ${location}`;
                        document.getElementById('temperature').textContent = `Temperature: ${temp.toFixed(1)}°C`;
                        document.getElementById('humidity').textContent = `Humidity: ${humidity}%`;
                    } else {
                        console.error(`Error: ${data.message}`);
                        document.getElementById('weather-icon').src = '';
                        document.getElementById('location').textContent = 'Location: Data not available';
                        document.getElementById('temperature').textContent = 'Temperature: Data not available';
                        document.getElementById('humidity').textContent = 'Humidity: Data not available';
                    }
                })
                .catch(error => {
                    console.error('Error fetching weather data:', error);
                    document.getElementById('weather-icon').src = '';
                    document.getElementById('location').textContent = 'Location: Error fetching data';
                    document.getElementById('temperature').textContent = 'Temperature: Error fetching data';
                    document.getElementById('humidity').textContent = 'Humidity: Error fetching data';
                });
        }

        updateWeather();
        setInterval(updateWeather, 600000);


        updateWeather();
        setInterval(updateWeather, 600000);
    }

    map.on('rotate', function() {
        const bearing = map.getBearing();
        const compass = document.getElementById('compass');
        compass.style.transform = `rotate(${bearing}deg)`;
    });
};
