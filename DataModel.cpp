#include "DataModel.h"

const std::array<thr::DataType, 5> thr::TypeConverter::DataTypes = {
	DataType{ QStringLiteral("xs:string"),			QStringLiteral("VARCHAR") },
	DataType{ QStringLiteral("xs:unsignedByte"),	QStringLiteral("BYTE") },
	DataType{ QStringLiteral("xs:unsignedShort"),	QStringLiteral("INTEGER") },
	DataType{ QStringLiteral("xs:unsignedInt"),		QStringLiteral("INTEGER") },
	DataType{ QStringLiteral("xs:decimal"),			QStringLiteral("MONEY") }	
};

class QString;
thr::DataType::DataType(QString&& pxsd, QString&& pmdb) :
	xsd{ pxsd }, mdb{ pmdb } {};

QString thr::TypeConverter::FromXSD(QString&& xsdType) {
	const auto p = std::find_if(beg, end, [xsdType](const DataType& d) { return d.xsd == xsdType; });
	if (p < end) {
		return p->mdb;
	} else {
		throw QStringLiteral("unknown data type: ") + xsdType;
	}
}

thr::AttParam::AttParam(QString&& attName, QString&& attType, bool attReq) :
	name{ attName }, type{ attType }, required{ attReq } {};


thr::DataModel::DataModel(QString& set, QString& item, AttList& al, ElemList& el) :
	setname{ std::move(set) }, itemname{ std::move(item) }, attlist{ std::move(al) }, elemlist{ std::move(el) } {
	assert(!setname.isEmpty());
	assert(!itemname.isEmpty());
	assert(!attlist.empty() || !elemlist.empty());
}

QString thr::DataModel::getSetName() const {
	return setname;
}
QString thr::DataModel::getItemName() const {
	return itemname;
}
const thr::AttList& thr::DataModel::getAttlist() const {
	return attlist;
}
const thr::ElemList& thr::DataModel::getElemlist() const {
	return elemlist;
}

void thr::DataModel::MakeStru(QSqlQuery& query) const {
	QString SQL{};
	SQL.reserve(256); //approx size
	for (const auto& elem : elemlist) {
		if (elem.second == ElemType::nestedSet) {
			SQL.resize(0);
			SQL += QStringLiteral("CREATE TABLE [") % itemname % elem.first % "]([pk] COUNTER PRIMARY KEY, [id] VARCHAR NOT NULL, [" % itemname % "] VARCHAR NOT NULL)";
			if (!query.exec(SQL)) {
				throw static_cast<QString>(QStringLiteral("Failed to create 8-8 table:\r\n") % query.lastQuery() % "\r\n" % query.lastError().text());
			}
		}

	}

	SQL.resize(0);
	SQL += QStringLiteral("CREATE TABLE [") % setname % "]([pk] COUNTER PRIMARY KEY, [";
	for (const auto& att : attlist) {
		SQL += att.name % "] " % att.type % (att.required ? " NOT NULL, [" : ", [");
	}
	for (const auto& elem : elemlist) {
		if (elem.second == ElemType::textElem) {
			SQL += elem.first % "] INTEGER NOT NULL, [";
		}
	}
	SQL.chop(3);
	SQL += ")";

	if (!query.exec(SQL)) {
		throw static_cast<QString>(QStringLiteral("Failed to create set table:\r\n") % query.lastQuery() % "\r\n" % query.lastError().text());
	}
}

QString thr::MakeSQL(const QString tablename, const AttList& attlist, const ElemList& elemlist, const ConstList& constlist) {
	const qsizetype n = attlist.size() + elemlist.size() + constlist.size();

	QString columns{};
	columns.reserve(20 * n); //approx total size
	QString values{};
	values.reserve(20 * n);

	for (const auto& p : attlist) {
		(columns += p.name) += "], [";
		(values += p.name) += ", :";
	}
	for (const auto& p : elemlist) {
		if (p.second == ElemType::textElem) {
			(columns += p.first) += "], [";
			(values += p.first) += ", :";
		}
	}
	for (const auto& p : constlist) {
		(columns += p.first) += "], [";
		(values += p.first) += ", :";
	}
	columns.chop(3);
	values.chop(3);

	return QStringLiteral("INSERT INTO [") % tablename % "] ([" % columns % ") VALUES (:" % values % ")";
}