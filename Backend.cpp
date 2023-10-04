#include "Backend.h"

#include <QtSql/qsqlerror.h>
#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlrecord.h>
#include <qxmlstream.h>
#include <qstring.h>
#include <qstringbuilder.h>
#include <qfile.h>

constexpr auto MY_CONN_NAME{ "therian_access_database" };
thr::Backend::Backend(const QString fullpath) : QObject{ nullptr } {
	if (!QSqlDatabase::contains(MY_CONN_NAME)) {
        auto conn{ QSqlDatabase::addDatabase(QStringLiteral("QODBC"), MY_CONN_NAME) };
		conn.setDatabaseName(QStringLiteral("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=") += fullpath);
	}
}
/*
void CopyElement(QXmlStreamReader& reader, QXmlStreamWriter& writer, int level) {
    while (reader.readNextStartElement()) {
        bool skip{ QStringLiteral("complexType") == reader.name() || QStringLiteral("sequence") == reader.name() };
        if (QStringLiteral("simpleContent") == reader.name()) {
            writer.writeStartElement(reader.name().toString());
            writer.writeEndElement();
            reader.skipCurrentElement();
            continue;
        }
        if (!skip) {
            writer.writeStartElement(reader.name().toString());
            for (const auto& att : reader.attributes()) {
                if (QStringLiteral("minOccurs") != att.name() && QStringLiteral("maxOccurs") != att.name()) {
                    writer.writeAttribute(att);
                }
            }
        }
        CopyElement(reader, writer, level+1);
        if (!skip) {
            writer.writeEndElement();
        }
    }
}

QString ViolateXSD() {
    QFile ifile{ QStringLiteral("E:\\GameData.xsd") };
    if (!ifile.open(QIODeviceBase::ExistingOnly | QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
        return QStringLiteral("Failed to open file [E:\\GameData.xsd]: ") + ifile.errorString();
    }

    QFile ofile{ QStringLiteral("E:\\T1.xsd") };
    if (!ofile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Unbuffered | QIODeviceBase::Text)) {
        return QStringLiteral("Failed to open file [E:\\T1.xsd]: ") + ofile.errorString();
    }

    QXmlStreamReader reader{ &ifile };
    QXmlStreamWriter writer{ &ofile };
    writer.setAutoFormatting(true);
    CopyElement(reader, writer, 0);
    qDebug() << "done";
    ifile.close();
    ofile.close();

    if (reader.hasError()) {
        return reader.errorString();
    }
    if (writer.hasError()) {
        return QStringLiteral("writer has error");
    }   
    return QStringLiteral("Done");
}
*/
void InnerElem(QXmlStreamReader& reader, thr::ElemList& list) {
    QString ElemName{ reader.attributes().value(QStringLiteral("name")).toString() };
    bool processed{ false };
    int level{ 1 };

    while (reader.readNextStartElement()) {
        level++;

        if (QStringLiteral("element") == reader.name()) {
            QStringView name{ reader.attributes().value(QStringLiteral("name")) };
            if (QStringLiteral("link") == name) {
                list.emplace_back(std::move(ElemName), thr::ElemType::nestedSet);
            } else if (QStringLiteral("text") == name) {
                list.emplace_back(std::move(ElemName), thr::ElemType::textElem);
            } else {
                throw QStringLiteral("Unexpected content") + reader.tokenString();
            }
            processed = true;
            break;
        }
    }
    if (!processed) {
        list.emplace_back(std::move(ElemName), thr::ElemType::emptyNode);
        --level;
    }
    while (0 < level--) {
        reader.skipCurrentElement();
    }
}

void InnerAttrib(QXmlStreamReader& reader, thr::AttList& list) {
    auto att{ reader.attributes() };
    QString name{ att.value(QStringLiteral("name")).toString() };
    QString type{ att.value(QStringLiteral("type")).toString() };
    QStringView req{ att.value(QStringLiteral("use")) };
    reader.skipCurrentElement();

    type = thr::TypeConverter::FromXSD(std::move(type));
    bool reqb{ QStringLiteral("required") == req };
    assert(reqb || QStringLiteral("optional") == req);

    list.emplace_back(std::move(name), std::move(type), reqb);
}

thr::ModelList thr::Backend::ReadSchema(const QString fullpath) {
    QFile ifile{ fullpath };
    if (!ifile.open(QIODeviceBase::ExistingOnly | QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
        throw static_cast<QString>(QStringLiteral("Failed to open file [") % fullpath % "]: " % ifile.errorString());
    }

    ModelList sets{};

    QXmlStreamReader reader{ &ifile };
    while (reader.readNextStartElement()) {
        if (QStringLiteral("element") == reader.name()){
            if (QStringLiteral("context") != reader.attributes().value(QStringLiteral("name"))) {
                throw QStringLiteral("Unexpected content") + reader.tokenString();
            }
            QString SetName{};
            QString ItemName{};
            ElemList elemlist{};
            AttList attlist{};

            while (reader.readNextStartElement()) {
                if (QStringLiteral("element") == reader.name()) {
                    SetName = reader.attributes().value(QStringLiteral("name")).toString();
                    int setlevel{ 1 };
                    
                    while (setlevel) {
                        if (reader.readNextStartElement()) {
                            if (QStringLiteral("element") == reader.name()) {
                                ItemName = reader.attributes().value(QStringLiteral("name")).toString();
                                int itemlevel{ 1 };

                                while (itemlevel) {
                                    if (reader.readNextStartElement()) {
                                        if (QStringLiteral("element") == reader.name()) {
                                            InnerElem(reader, elemlist);
                                        } else if (QStringLiteral("attribute") == reader.name()) {
                                            InnerAttrib(reader, attlist);
                                        } else {
                                            ++itemlevel;
                                        }
                                    } else {
                                        --itemlevel;
                                    }
                                }
                            } else {
                                ++setlevel;
                            }
                        } else {
                            --setlevel;
                        }
                    }

                    sets.emplace_back(SetName, ItemName, attlist, elemlist);
                }            
            }            
        }
    }

    return sets;
}

void thr::Backend::CreateStru(const QSqlDatabase& conn, const thr::ModelList& models) {
    QSqlQuery query{ conn };
    for (const auto& set : models) {
        set.MakeStru(query);
    }
    if (!query.exec(QStringLiteral("CREATE TABLE [texts](pk COUNTER PRIMARY KEY, fr MEMO, en MEMO, ru MEMO, de MEMO, it MEMO, pl MEMO, tr MEMO, es MEMO)"))) {
        throw static_cast<QString>(QStringLiteral("Failed to create texts table:\r\n") % query.lastQuery() % "\r\n" % query.lastError().text());
    }
}

QVariant ProcessTexts(QXmlStreamReader& reader, const QSqlDatabase& conn) {
    QSqlQuery qu{ conn };

    if (!qu.prepare(QStringLiteral("INSERT INTO [texts] ([fr], [en], [ru], [de], [it], [pl], [tr], [es]) VALUES (:fr, :en, :ru, :de, :it, :pl, :tr, :es)"))) {
        throw static_cast<QString>(QStringLiteral("failed to prepare query:\r\n") % qu.lastQuery() % "\r\n" % qu.lastError().text());
    }

    while (reader.readNextStartElement()) {
        if (QStringLiteral("text") == reader.name()) {
            QString c{};
            try {
                c = QStringLiteral(":") += reader.attributes().value(QStringLiteral("c"));
            } catch (...) {
                throw QStringLiteral("set doesnt have \"count\" attribute");
            }

            qu.bindValue(c, QVariant{ reader.readElementText(QXmlStreamReader::IncludeChildElements) });
        } else {
            throw QStringLiteral("Unexpected element in text node: ") += reader.name();
        }
    }

    if (!qu.exec()) {
        throw static_cast<QString>(QStringLiteral("Failed to exec:\r\n") % qu.lastQuery() % "\r\n" % qu.lastError().text());
    }

    if (!qu.exec(QStringLiteral("SELECT @@IDENTITY"))) {
        throw QStringLiteral("Failed to get identity: ") + qu.lastError().text();
    }

    qu.first();
    return qu.record().value(0);
}

namespace thr {
    const AttList nestedTable = {
        AttParam{ QStringLiteral("id"), QStringLiteral("VARCHAR"), true }
    };
}

void ProcessItemSet(QXmlStreamReader& reader, const QSqlDatabase& conn, const QString ItemName, const QString TableName,
const thr::AttList& attlist, const thr::ElemList& elemlist, const thr::ConstList& constlist) {
    int expected{ 0 };
    int processed{ 0 };
    
    try {
        expected = reader.attributes().value(QStringLiteral("count")).toLong();
    } catch (...) {
        throw QStringLiteral("set doesnt have \"count\" attribute");
    }
    
    QSqlQuery qu{ conn };
    if (!qu.prepare(thr::MakeSQL(TableName, attlist, elemlist, constlist))) {
        throw static_cast<QString>(QStringLiteral("failed to prepare query:\r\n") % qu.lastQuery() % "\r\n" % qu.lastError().text());
    }
    
    //process constlist
    if (!constlist.empty()) {
        int bi = attlist.size() + elemlist.size();
        for (const auto& p : constlist) {
            qu.bindValue(bi++, p.second);
        }
    }
        
    while (reader.readNextStartElement()) {      
        if (ItemName == reader.name()) {
            //Bind index
            int bi{ 0 };

            //process attributes
            if (!attlist.empty()) { 
                auto xmlattrib = reader.attributes();
                for (const auto& att : attlist) {
                    const auto q = xmlattrib.end();
                    const auto p = std::find_if(xmlattrib.begin(), q, [&att](const auto& xml) { return att.name == xml.name(); });
                    if (p < q) {
                        qDebug() << "[" << bi << "]: " << att.name << "; " << p->name() << " = " << p->value();
                        qu.bindValue(bi++, p->value().toString());
                        xmlattrib.erase(p);
                    } else {
                        if (att.required) {
                            throw QStringLiteral("Missing expected attribute: ") += att.name;
                        } else {
                            qDebug() << "[" << bi << "]: " << att.name << " is missing";
                            qu.bindValue(bi++, QVariant{});
                        }
                    }
                }
                if (!xmlattrib.empty()) {
                    throw QStringLiteral("Unexpected attribute: ") += xmlattrib.begin()->name();
                }
            }

            //process elements
            if (!elemlist.empty()) {
                QVariant id{ reader.attributes().value(QStringLiteral("id")).toString() };

                for (const auto& elem : elemlist) {
                    if (!reader.readNextStartElement()) {
                        throw static_cast<QString>(QStringLiteral("Unexpected end of element [") % ItemName % "].Expected " % elem.first);
                    }
                    qDebug() << reader.name();
                    if (elem.first == reader.name()) {
                        switch (elem.second) {
                        case thr::ElemType::nestedSet:
                            if (id.isNull()) {
                                throw QStringLiteral("There is no id but nested list excepted: ") += ItemName;
                            }
                            ProcessItemSet(reader, conn,
                                QStringLiteral("link"),
                                ItemName + elem.first,
                                thr::nestedTable, {},
                                { { ItemName, id } });
                            break;
                        case thr::ElemType::textElem:
                            try {
                                qDebug() << "[" << bi << "]: " << elem.first;
                                qu.bindValue(bi++, ProcessTexts(reader, conn));
                            } catch (QString ex) {
                                throw static_cast<QString>(QStringLiteral("error binding text element [") % elem.first % "]: " % ex);
                            }
                            break;
                        case thr::ElemType::emptyNode:
                            if (reader.readNextStartElement()) {
                                throw static_cast<QString>(QStringLiteral("Element is not empty: ") % ItemName % "->" % elem.first);
                            }
                            break;
                        }
                    } else {
                        throw static_cast<QString>(QStringLiteral("Unexpected element [") % ItemName % "." % reader.name() % "]. Expected " % elem.first);
                    }
                }
            }

            if (!qu.exec()) {
                throw static_cast<QString>(QStringLiteral("Failed to exec:\r\n") % qu.lastQuery() % "\r\n" % qu.lastError().text());
            }
            
            processed++;

            if (reader.readNextStartElement()) {
                throw static_cast<QString>(QStringLiteral("Unexpected content in [") % ItemName % "]: " % reader.name());
            }
        } else {
            // reader.skipCurrentElement();
            throw QStringLiteral("Unexpected element: ") += reader.name();
        }
    }

    if (expected != processed) {
        throw static_cast<QString>(QStringLiteral("Wrong number of elements in set: ") % ItemName % " expected " % QString::number(expected) % " processed " % QString::number(processed));
    }   
}

void thr::Backend::ReadXML(const QString fullname, const QSqlDatabase& conn, const ModelList& sets) {
    QFile xfile{ fullname };
    if (!xfile.open(QIODeviceBase::ExistingOnly | QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
        throw static_cast<QString>(QStringLiteral("Failed to load from file [") % fullname % "]: " % xfile.errorString());
    }

    const auto set_beg = sets.begin();
    const auto set_end = sets.end();

    QXmlStreamReader reader{ &xfile };
    while (reader.readNextStartElement()) {
        if (QStringLiteral("context") == reader.name()) {
            while (reader.readNextStartElement()) {
                const auto nameOfSet{ reader.name() };
                qDebug() << nameOfSet; //theese are sets at this level
                try {
                    const auto& set = std::find_if(set_beg, set_end, [&nameOfSet](const DataModel& m) { return m.getSetName() == nameOfSet; });
                    if (set < set_end) {
                        ProcessItemSet(reader, conn, set->getItemName(), set->getSetName(), set->getAttlist(), set->getElemlist(), {});
                    } else {
                        throw QStringLiteral("unknown set");
                    }

                    /*
                    if (QStringLiteral("skillSet") == nameOfSet) {
                        ProcessItemSet(reader, conn, QStringLiteral("skill"), nameOfSet, {
                            { QStringLiteral("sequenceOrder"), false },
                            { QStringLiteral("iconId"), false },
                            { QStringLiteral("id"), true },
                            { QStringLiteral("isHidden"), false }
                        }, {
                            { QStringLiteral("usedByItemTypes"), thr::ElemType::nestedSet },
                            { QStringLiteral("usedByUnitBaseSkills"), thr::ElemType::nestedSet },
                            { QStringLiteral("usedByItemBaseSkills"), thr::ElemType::nestedSet },
                            { QStringLiteral("subSkills"), thr::ElemType::nestedSet },
                            { QStringLiteral("taskGroups"), thr::ElemType::nestedSet },
                            { QStringLiteral("name"), thr::ElemType::textElem }
                        }, {});
                    } else
                    */
                } catch (QString ex) {
                    throw static_cast<QString>(QStringLiteral("[") % nameOfSet % "]: " % ex);
                }
            }
        }
    }
    xfile.close();

    if (reader.hasError() && reader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        throw reader.errorString();
    }
}

QString thr::Backend::test(const QString schema, const QString XML) {
    ModelList models{};

    try {
        models = ReadSchema(schema);
    } catch (const QString ex) {
        return QStringLiteral("Failed to read xsd: ") + ex;
    }

    auto conn{ QSqlDatabase::database(MY_CONN_NAME) };
    if (conn.open()) {
        CreateStru(conn, models);

        try {
            ReadXML(XML, conn, models);
            conn.close();
        } catch (const QString ex) {
            conn.close();
            return QStringLiteral("Failed to read xml: ") + ex;
        }
        
        return QStringLiteral("OK");
    } else {
        return conn.lastError().text();
    }
}