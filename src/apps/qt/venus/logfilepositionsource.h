// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef LOGFILEPOSITIONSOURCE_H
#define LOGFILEPOSITIONSOURCE_H

#define DB_TIME_COLUMN          (40)
#define DB_SPEED_COLUMN             (55)
#define DB_HEADING_COLUMN       (56)
#define DB_LATITUDE_COLUMN      (62)
#define DB_LONGITUDE_COLUMN     (63)
#define DB_DEVICEID_COLUMN      (47)
#define DB_PDR_COLUMN               (68)
#define DB_DISTANCE_COLUMN          (61)

#define DB_CV_HEADING_COLUMN       (38)
#define DB_CV_LATITUDE_COLUMN      (32)
#define DB_CV_LONGITUDE_COLUMN     (33)
#define DB_CV_DEVICEID_COLUMN      (17)

#include <QtPositioning/qgeopositioninfosource.h>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTimer>

QT_BEGIN_NAMESPACE
//class QFile;
//class QTimer;
class QGeoPositionInfoSource;
class QGeoPositionInfo;
QT_END_NAMESPACE

typedef struct GPS_V2X_POSITION_t {
    double dLatitude;
    double dLongitude;
    double dAttitude;
} GPS_POSITION_T;

class LogFilePositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT
    Q_PROPERTY(double pdr READ getGpsPdr NOTIFY pdrUpdated)

public:
    explicit LogFilePositionSource(QObject *parent = 0);

    QGeoPositionInfo lastKnownPosition(bool satelliteMethodsOnly = false) const override;

    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

public slots:
    virtual void startUpdates() override;
    virtual void stopUpdates() override;
    virtual void requestUpdate(int timeout = 5000) override;

    virtual unsigned int updateGpsPosition(void);

    virtual unsigned int getGpsSpeed(void);
    virtual unsigned int getGpsDistance(void);

    virtual unsigned int getGpsHeading(void);

    virtual double getGpsPdr(void);

    virtual double getGpsLatitude(void);
    virtual double getGpsLongitude(void);
    virtual QString getGpsDeviceId(void);

    virtual unsigned int getGpsCvHeading(void);
    virtual double  getGpsCvLatitude(void);
    virtual double  getGpsCvLongitude(void);
    virtual QString getGpsCvDeviceId(void);
    virtual QString getTimestamp(void);

signals:
    void positionChanged();
    void pdrUpdated(double pdr);

private slots:
    void readNextPosition();

private:
    QFile *logFile;
    QTimer *timer;
    QGeoPositionInfo lastPosition;
    Error lastError = QGeoPositionInfoSource::NoError;
    double m_pdr;
};

#endif
