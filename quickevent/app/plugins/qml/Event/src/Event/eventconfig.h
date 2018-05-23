#ifndef EVENTCONFIG_H
#define EVENTCONFIG_H

#include "../eventpluginglobal.h"

#include <QObject>
#include <QVariantMap>
#include <QSet>
#include <QDateTime>

namespace Event {

class EVENTPLUGIN_DECL_EXPORT EventConfig : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int stageCount READ stageCount)
	Q_PROPERTY(bool isHandicap READ isHandicap)
	//Q_PROPERTY(QString eventName READ eventName)
public:
	enum class Sport {OB = 1, LOB, MTBO, TRAIL};
	enum class Discipline {Classic = 1, ShortRace, Sprint,
						   Relays = 5,
						   NightRace = 9,
						  };
public:
	explicit EventConfig(QObject *parent = 0);
public slots:
	QVariantMap values() const {return m_data;}
	//void setValues(const QVariantMap &vals);
	QVariant value(const QStringList &path, const QVariant &default_value = QVariant()) const;
	QVariant value(const QString &path, const QVariant &default_value = QVariant()) const {return value(path.split('.'), default_value);}
	void setValue(const QStringList &path, const QVariant &val);
	void setValue(const QString &path, const QVariant &val) {setValue(path.split('.'), val);}
	void load();
	void save(const QString &path_to_save = QString());

	int stageCount() const;
	int currentStageId() const;
	int sportId() const;
	int disciplineId() const;
	int importId() const;
	int handicapLength() const;
	bool isHandicap() const {return handicapLength() > 0;}
	bool isRelays() const {return disciplineId() == (int)Discipline::Relays;}
	QString director() const;
	QString mainReferee() const;
	QDateTime eventDateTime() const;
	int dbVersion() const;
private:
	void save_helper(QVariantMap &ret, const QString &current_path, const QVariant &val);
	QVariantMap setValue_helper(const QVariantMap &m, const QStringList &path, const QVariant &val);
private:
	QVariantMap m_data;
};

}

#endif // EVENTCONFIG_H
