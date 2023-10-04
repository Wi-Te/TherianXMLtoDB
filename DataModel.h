#pragma once
#include <qstring.h>
#include <qstringbuilder.h>
#include <qvariant.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlerror.h>

namespace thr {
	enum class ElemType {
		nestedSet,
		emptyNode,
		textElem
	};	

	struct DataType {
		QString xsd;
		QString mdb;

		explicit DataType() = delete;
		DataType(QString&&, QString&&);
	};

	class TypeConverter {
	private:
		static const std::array<DataType, 5> DataTypes;
		static constexpr auto beg = DataTypes.begin();
		static constexpr auto end = DataTypes.end();
	public:
		static QString FromXSD(QString&&);
	};	

	struct AttParam {
		QString name;
		QString type;
		bool required;
		explicit AttParam() = delete;
		AttParam(QString&&, QString&&, bool);
	};

	typedef std::vector<AttParam> AttList;
	typedef std::vector<std::pair<QString, ElemType>> ElemList;
	typedef std::vector<std::pair<QString, const QVariant&>> ConstList;	

	class DataModel {
	private:
		AttList attlist;
		ElemList elemlist;
		QString itemname;
		QString setname;
	public:
		explicit DataModel() = delete;
		DataModel(QString& set, QString& item, AttList& al, ElemList& el);

		QString getSetName() const;
		QString getItemName() const;
		const AttList& getAttlist() const;
		const ElemList& getElemlist() const;
		void MakeStru(QSqlQuery& query) const;
	};

	QString MakeSQL(const QString tablename, const AttList& attlist, const ElemList& elemlist, const ConstList& constlist);
	typedef std::vector<DataModel> ModelList;
}