#pragma once

#include <QObject>
#include <QQmlEngine>
#include "DataModel.h"

class QString;
class QSqlDatabase;

namespace thr {
	class Backend : public QObject {
		Q_OBJECT
		QML_ELEMENT
	private:
		ModelList ReadSchema(const QString);
		void CreateStru(const QSqlDatabase&, const ModelList&);
		void ReadXML(const QString, const QSqlDatabase&, const ModelList&);
	public:
		explicit Backend(const QString);
		Q_INVOKABLE QString test(const QString, const QString);
	};
}