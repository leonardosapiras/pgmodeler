/*
# PostgreSQL Database Modeler (pgModeler)
#
# Copyright 2006-2013 - Raphael Araújo e Silva <rkhaotix@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The complete text of GPLv3 is at LICENSE file on source code root directory.
# Also, you can get the complete GNU General Public License at <http://www.gnu.org/licenses/>
*/

#include "baseobject.h"
#include <QApplication>

/* CAUTION: If both amount and order of the enumerations are modified
	 then the order and amount of the elements of this vector
	 must also be modified */
QString BaseObject::objs_schemas[OBJECT_TYPE_COUNT]={
	"column",  "constraint", "function", "trigger",
	"index", "rule", "table", "view",
	"domain", "schema", "aggregate", "operator",
	"sequence", "role", "conversion", "cast",
	"language", "usertype", "tablespace",
	"opfamily", "opclass", "database","collation",
	"extension", "relationship","textbox",	"permission",
	"parameter", "typeattribute","relationship"
};

QString BaseObject::obj_type_names[OBJECT_TYPE_COUNT]={
	QT_TR_NOOP("Column"), QT_TR_NOOP("Constraint"), QT_TR_NOOP("Function"),
	QT_TR_NOOP("Trigger"), QT_TR_NOOP("Index"), QT_TR_NOOP("Rule"),
	QT_TR_NOOP("Table"), QT_TR_NOOP("View"),  QT_TR_NOOP("Domain"),
	QT_TR_NOOP("Schema"), QT_TR_NOOP("Aggregate"), QT_TR_NOOP("Operator"),
	QT_TR_NOOP("Sequence"), QT_TR_NOOP("Role"), QT_TR_NOOP("Conversion"),
	QT_TR_NOOP("Cast"), QT_TR_NOOP("Language"), QT_TR_NOOP("Type"), QT_TR_NOOP("Tablespace"),
	QT_TR_NOOP("Operator Family"), QT_TR_NOOP("Operator Class"),
	QT_TR_NOOP("Database"), QT_TR_NOOP("Collation"), QT_TR_NOOP("Extension"),
	QT_TR_NOOP("Relationship"),	QT_TR_NOOP("Textbox"), QT_TR_NOOP("Permission"),
	QT_TR_NOOP("Parameter"), QT_TR_NOOP("Type Attribute"), QT_TR_NOOP("Basic Relationship")
};

QString BaseObject::objs_sql[OBJECT_TYPE_COUNT]={
	"COLUMN",  "CONSTRAINT", "FUNCTION", "TRIGGER",
	"INDEX", "RULE", "TABLE", "VIEW",
	"DOMAIN", "SCHEMA", "AGGREGATE", "OPERATOR",
	"SEQUENCE", "ROLE", "CONVERSION", "CAST",
	"LANGUAGE", "TYPE", "TABLESPACE",
	"OPERATOR FAMILY", "OPERATOR CLASS", "DATABASE",
	"COLLATION", "EXTENSION"
};

/* Initializes the global id which is shared between instances
	 of classes derived from the this class. The value of global_id
	 starts at 70k because the id ranges 0, 10k, 20k, 30k, 40k, 50k, 60k
	 are respectively assigned to objects of classes Role, Tablespace
	 DBModel, Schema, Collation, Function and Type */
//unsigned BaseObject::global_id=60000;
unsigned BaseObject::global_id=30000;

BaseObject::BaseObject(void)
{
	object_id=BaseObject::global_id++;
	is_protected=system_obj=sql_disabled=false;
	obj_type=BASE_OBJECT;
	schema=nullptr;
	owner=nullptr;
	tablespace=nullptr;
	database=nullptr;
	collation=nullptr;
	attributes[ParsersAttributes::NAME]="";
	attributes[ParsersAttributes::COMMENT]="";
	attributes[ParsersAttributes::OWNER]="";
	attributes[ParsersAttributes::TABLESPACE]="";
	attributes[ParsersAttributes::SCHEMA]="";
	attributes[ParsersAttributes::COLLATION]="";
	attributes[ParsersAttributes::PROTECTED]="";
	attributes[ParsersAttributes::SQL_DISABLED]="";
	this->setName(QApplication::translate("BaseObject","new_object","", -1));
}

unsigned BaseObject::getGlobalId(void)
{
	return(global_id);
}

QString BaseObject::getTypeName(ObjectType obj_type)
{
	if(obj_type!=BASE_OBJECT)
		/* Due to the class BaseObject not be derived from QObject the function tr() is inefficient to
		 translate the type names thus the method called to do the translation is from the application
		 specifying the context (BaseObject) in the ts file and the text to be translated */
		return(QApplication::translate("BaseObject",obj_type_names[obj_type].toStdString().c_str(),"", -1));
	else
		return("");
}

QString BaseObject::getSchemaName(ObjectType obj_type )
{
	return(objs_schemas[obj_type ]);
}

QString BaseObject::getSQLName(ObjectType obj_type)
{
	return(objs_sql[obj_type]);
}

QString BaseObject::formatName(const QString &name, bool is_operator)
{
	int i;
	bool is_formated=false;
	QString frmt_name;
	QByteArray raw_name;
	unsigned char chr, chr1, chr2;
	QRegExp regexp_vect[]={
		QRegExp("(\")(.)+(\")"),
		QRegExp("(\")(.)+(\")(\\.)(\")(.)+(\")"),
		QRegExp("(\")(.)+(\")(\\.)(.)+"),
		QRegExp("(.)+(\\.)(\")(.)+(\")"),
		QRegExp("(.)+(\\.)(.)+")
	};

	/* Checks through regular expressions
		if the name passed to be formatted is yet
		formatted. The forms likely to be formatted are:

		1) "OBJECT_NAME"
		2) "SCHEMA_NAME"."OBJECT_NAME"
		3) "SCHEMA_NAME".OBJECT_NAME
		4) SCHEMA_NAME."OBJECT_NAME"
		5) SCHEMA_NAME.OBJECT_NAME */
	for(i=0; i < 5 && !is_formated; i++)
		is_formated=regexp_vect[i].exactMatch(name);

	/* If the name is not formatted or it symbolizes the name of an operator
		(which has characters invalid according to the rule and is the only exception
		 to which its name is formatted even being invalid) or if the name is valid according
		 with PostgreSQL rules for other types of objects */
	if(!is_formated && (is_operator || isValidName(name)))
	{
		bool is_upper=false;
		unsigned i, qtd;

		raw_name.append(name);

		/* Checks if the name has some upper case letter. If its the
		 case the name will be enclosed in quotes */
		qtd=name.size();

		i=0;
		while(i < qtd && !is_upper)
		{
			chr=raw_name[i];

			if(((i + 1) < (qtd-1)) &&
				 ((chr >= 0xC2 && chr <= 0xDF) ||
					(chr >= 0xE0 && chr <= 0xEF)))
				chr1=raw_name[i+1];
			else
				chr1=0;

			if((i + 2) < (qtd-1) &&
				 chr >= 0xE0 && chr <= 0xEF)
				chr2=raw_name[i+2];
			else
				chr2=0;

			if(chr1!=0 && chr2!=0)
				i+=3;
			else if(chr1!=0 && chr2==0)
				i+=2;
			else
				i++;

			//2 bytes UTF-8 character
			if((chr  >= 0xC2 && chr <= 0xDF &&
					chr1 >= 0x80 && chr1 <= 0xBF) ||

				 //3 bytes UTF-8 character
				 (chr  >= 0xE0 && chr <= 0xEF &&
					chr1 >= 0x80 && chr1 <= 0xBF &&
					chr2 >= 0x80 && chr2 <= 0xBF) ||

				 QChar(chr).isUpper())
			{
				is_upper=true;
			}

		}

		if(is_upper)
			frmt_name="\"" + name + "\"";
		else
			frmt_name=name;
	}
	else if(is_formated)
		frmt_name=name;

	return(frmt_name);
}

bool BaseObject::isValidName(const QString &name)
{
	int len;
	QByteArray raw_name;

	raw_name.append(name);
	len=raw_name.size();

	/* If the name is greater than the maximum size accepted
		by PostgreSQL (currently 63 bytes) or it is empty
		the name is invalid */
	if(raw_name.isEmpty() || len > OBJECT_NAME_MAX_LENGTH)
		return(false);
	else
	{
		int i=0;
		bool valid=true;
		unsigned char chr='\0', chr1='\0', chr2='\0';

		chr=raw_name[0];
		if(len > 1)
			chr1=raw_name[len-1];

		//Checks if the name is enclosed in quotes
		if(chr=='\"' && chr1=='\"')
		{
			/* Validates the name but the validation will continue until the
			end of string (or the last quote) */
			valid=true; i++; len--;
		}

		while(valid && i < len)
		{
			chr=raw_name[i];

			/* Validation of simple ASCI characters.
			Checks if the name has the characters in the set [ a-z A-Z 0-9 _ ] */
			if((chr >= 'a' && chr <='z') ||
				 (chr >= 'A' && chr <='Z') ||
				 (chr >= '0' && chr <='9') ||
				 chr == '_')
			{
				valid=true;
				i++;
			}
			else valid=false;

			/* Validation of UTF8 charactes (2 and 3 bytes long).
			Reference: http://www.fileformat.info/info/unicode/utf8.htm
								 http://en.wikipedia.org/wiki/UTF-8

			Snippet extracted from the above url:

			The value of each individual byte indicates its UTF-8 function, as follows:
			00 to 7F hex (0 to 127): first and only byte of a sequence.
			80 to BF hex (128 to 191): continuing byte in a multi-byte sequence.
			C2 to DF hex (194 to 223): first byte of a two-byte sequence.
			E0 to EF hex (224 to 239): first byte of a three-byte sequence.  */
			if(!valid && (i < len-1))
			{
				chr1=raw_name[i+1];

				if((i + 2) <= (len-1))
					chr2=raw_name[i+2];
				else
					chr2=0;

				//UTF-8 character with 2 bytes length
				if((chr  >= 0xC2 && chr <= 0xDF &&
						chr1 >= 0x80 && chr1 <= 0xBF) ||

					 //UTF-8 character with 3 bytes length
					 (chr  >= 0xE0 && chr <= 0xEF &&
						chr1 >= 0x80 && chr1 <= 0xBF &&
						chr2 >= 0x80 && chr2 <= 0xBF))
					valid=true;

				//Increments the counter in the size of the validated char
				if(chr >= 0xC2 && chr <= 0xDF)
					//2 bytes char
					i+=2;
				else
					//3 bytes char
					i+=3;
			}
		}

		return(valid);
	}
}

void BaseObject::setDatabase(BaseObject *db)
{
	if((db && db->getObjectType()==OBJ_DATABASE) || !db)
		this->database=db;
}

BaseObject *BaseObject::getDatabase(void)
{
	return(this->database);
}

void BaseObject::setProtected(bool value)
{
	is_protected=(!system_obj ? value : true);
}

void BaseObject::setName(const QString &name)
{
	//Raises an error if the passed name is empty
	if(name.isEmpty())
		throw Exception(ERR_ASG_EMPTY_NAME_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else
	{
		int count;
		QString aux_name=name;

		count=aux_name.count(QChar('\0'));
		if(count >=1) aux_name.chop(count);

		//Raises an error if the passed name is invalid
		if(!isValidName(aux_name))
		{
			if(name.size() > OBJECT_NAME_MAX_LENGTH)
				throw Exception(ERR_ASG_LONG_NAME_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
			else
				throw Exception(ERR_ASG_INV_NAME_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
		}
		else
		{
			aux_name.remove('\"');
			this->obj_name=aux_name;
		}
	}
}

void BaseObject::setComment(const QString &comment)
{
	this->comment=comment;
}

bool BaseObject::acceptsSchema(ObjectType obj_type)
{
	return(obj_type==OBJ_FUNCTION || obj_type==OBJ_TABLE ||
				 obj_type==OBJ_VIEW  || obj_type==OBJ_DOMAIN ||
				 obj_type==OBJ_AGGREGATE || obj_type==OBJ_OPERATOR ||
				 obj_type==OBJ_SEQUENCE || obj_type==OBJ_CONVERSION ||
				 obj_type==OBJ_TYPE || obj_type==OBJ_OPCLASS ||
				 obj_type==OBJ_OPFAMILY || obj_type==OBJ_COLLATION ||
				 obj_type==OBJ_EXTENSION);
}

bool BaseObject::acceptsSchema(void)
{
	return(BaseObject::acceptsSchema(this->obj_type));
}

bool BaseObject::acceptsOwner(ObjectType obj_type)
{
	return(obj_type==OBJ_FUNCTION || obj_type==OBJ_TABLE ||
				 obj_type==OBJ_DOMAIN || obj_type==OBJ_SCHEMA ||
				 obj_type==OBJ_AGGREGATE || obj_type==OBJ_OPERATOR ||
				 obj_type==OBJ_CONVERSION ||
				 obj_type==OBJ_LANGUAGE || obj_type==OBJ_TYPE ||
				 obj_type==OBJ_TABLESPACE || obj_type==OBJ_DATABASE ||
				 obj_type==OBJ_OPCLASS || obj_type==OBJ_OPFAMILY ||
				 obj_type==OBJ_COLLATION);
}

bool BaseObject::acceptsOwner(void)
{
	return(BaseObject::acceptsOwner(this->obj_type));
}

bool BaseObject::acceptsTablespace(ObjectType obj_type)
{
	return(obj_type==OBJ_INDEX ||
				 obj_type==OBJ_TABLE ||
				 obj_type==OBJ_CONSTRAINT ||
				 obj_type==OBJ_DATABASE);
}

bool BaseObject::acceptsTablespace(void)
{
	return(BaseObject::acceptsTablespace(this->obj_type));
}

bool BaseObject::acceptsCollation(ObjectType obj_type)
{
	return(obj_type==OBJ_DOMAIN || obj_type==OBJ_COLUMN  ||
				 obj_type==OBJ_COLLATION || obj_type==OBJ_TYPE ||
				 obj_type==OBJ_TYPE_ATTRIBUTE);
}

bool BaseObject::acceptsCollation(void)
{
	return(BaseObject::acceptsCollation(this->obj_type));
}

void BaseObject::setSchema(BaseObject *schema)
{
	if(!schema)
		throw Exception(Exception::getErrorMessage(ERR_ASG_NOT_ALOC_SCHEMA)
										.arg(Utf8String::create(this->obj_name)).arg(this->getTypeName()),
										ERR_ASG_NOT_ALOC_SCHEMA,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else if(schema && schema->getObjectType()!=OBJ_SCHEMA)
		throw Exception(ERR_ASG_INV_SCHEMA_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else if(!acceptsSchema())
		throw Exception(ERR_ASG_INV_SCHEMA_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);

	this->schema=schema;
}

void BaseObject::setOwner(BaseObject *owner)
{
	if(owner && owner->getObjectType()!=OBJ_ROLE)
		throw Exception(ERR_ASG_INV_ROLE_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else if(!acceptsOwner())
		throw Exception(ERR_ASG_ROLE_OBJECT_INV_TYPE,__PRETTY_FUNCTION__,__FILE__,__LINE__);

	this->owner=owner;
}

void BaseObject::setTablespace(BaseObject *tablespace)
{
	if(tablespace && tablespace->getObjectType()!=OBJ_TABLESPACE)
		throw Exception(ERR_ASG_INV_TABLESPACE_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else if(!acceptsTablespace())
		throw Exception(ERR_ASG_TABSPC_INV_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);

	this->tablespace=tablespace;
}

void BaseObject::setCollation(BaseObject *collation)
{
	if(collation && !acceptsCollation())
		throw Exception(ERR_ASG_INV_COLLATION_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	if(collation && collation->getObjectType()!=OBJ_COLLATION)
		throw Exception(ERR_ASG_INV_COLLATION_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);

	this->collation=collation;
}

QString BaseObject::getName(bool format, bool prepend_schema)
{
	if(format)
	{
		QString aux_name;

		aux_name=formatName(this->obj_name, (obj_type==OBJ_OPERATOR));

		if(this->schema && prepend_schema)
			aux_name=formatName(this->schema->getName()) + "." + aux_name;

		return(aux_name);
	}
	else return(this->obj_name);
}

QString BaseObject::getComment(void)
{
	return(comment);
}

BaseObject *BaseObject::getSchema(void)
{
	return(schema);
}

BaseObject *BaseObject::getOwner(void)
{
	return(owner);
}

BaseObject *BaseObject::getTablespace(void)
{
	return(tablespace);
}

BaseObject *BaseObject::getCollation(void)
{
	return(collation);
}

ObjectType BaseObject::getObjectType(void)
{
	return(obj_type);
}

QString BaseObject::getTypeName(void)
{
	return(BaseObject::getTypeName(this->obj_type));
}

QString BaseObject::getSchemaName(void)
{
	return(BaseObject::getSchemaName(this->obj_type));
}

QString BaseObject::getSQLName(void)
{
	return(BaseObject::getSQLName(this->obj_type));
}

bool BaseObject::isProtected(void)
{
	return(is_protected);
}

unsigned BaseObject::getObjectId(void)
{
	return(object_id);
}

bool BaseObject::operator == (const QString &name)
{
	return(this->obj_name==name);
}

bool BaseObject::operator != (const QString &name)
{
	return(this->obj_name!=name);
}

void BaseObject::setSQLDisabled(bool value)
{
	sql_disabled=value;
}

bool BaseObject::isSQLDisabled(void)
{
	return(sql_disabled);
}

void BaseObject::setSystemObject(bool value)
{
	system_obj=sql_disabled=is_protected=value;
}

bool BaseObject::isSystemObject(void)
{
	return(system_obj);
}

QString BaseObject::__getCodeDefinition(unsigned def_type)
{
	return(BaseObject::getCodeDefinition(def_type, false));
}

QString BaseObject::getCodeDefinition(unsigned def_type, bool reduced_form)
{
	QString code_def;

	if((def_type==SchemaParser::SQL_DEFINITION &&
			obj_type!=BASE_OBJECT && obj_type!=BASE_RELATIONSHIP &&
			obj_type!=BASE_TABLE && obj_type!=OBJ_TEXTBOX) ||

		 (def_type==SchemaParser::XML_DEFINITION &&
			obj_type!=BASE_OBJECT && obj_type!=BASE_TABLE))
	{
		bool format;

		attributes[ParsersAttributes::SQL_DISABLED]=(sql_disabled ? "1" : "");

		//Formats the object's name in case the SQL definition is being generated
		format=((def_type==SchemaParser::SQL_DEFINITION) ||
						(def_type==SchemaParser::XML_DEFINITION && reduced_form &&
						 obj_type!=OBJ_TEXTBOX && obj_type!=OBJ_RELATIONSHIP));

		/* Marking the flag that indicates that the comment form to be generated
		 for the object is specific to it, ignoring the default rule.
		 (See SQL schema file for comments) */
		switch(obj_type)
		{
			case OBJ_COLUMN:
			case OBJ_AGGREGATE:
			case OBJ_FUNCTION:
			case OBJ_CAST:
			case OBJ_CONSTRAINT:
			case OBJ_RULE:
			case OBJ_TRIGGER:
			case OBJ_OPERATOR:
			case OBJ_OPCLASS:
			case OBJ_OPFAMILY:
				attributes[ParsersAttributes::DIF_SQL]="1";
				attributes[objs_schemas[obj_type]]="1";
			break;

			default:
				attributes[ParsersAttributes::DIF_SQL]="";
			break;
		}

		if(attributes[ParsersAttributes::NAME].isEmpty())
			attributes[ParsersAttributes::NAME]=this->getName(format);

		attributes[ParsersAttributes::SQL_OBJECT]=objs_sql[this->obj_type];

		if(schema)
		{
			if(def_type==SchemaParser::XML_DEFINITION)
				attributes[ParsersAttributes::SCHEMA]=schema->getCodeDefinition(def_type, true);
			else
				attributes[ParsersAttributes::SCHEMA]=schema->getName(format);
		}

		if(def_type==SchemaParser::XML_DEFINITION)
			attributes[ParsersAttributes::PROTECTED]=(is_protected ? "1" : "");

		if(tablespace)
		{
			if(def_type==SchemaParser::SQL_DEFINITION)
				attributes[ParsersAttributes::TABLESPACE]=tablespace->getName(format);
			else
				attributes[ParsersAttributes::TABLESPACE]=tablespace->getCodeDefinition(def_type, true);
		}

		if(collation && attributes[ParsersAttributes::COLLATION].isEmpty())
		{
			if(def_type==SchemaParser::SQL_DEFINITION)
				attributes[ParsersAttributes::COLLATION]=collation->getName(format);
			else
				attributes[ParsersAttributes::COLLATION]=collation->getCodeDefinition(def_type, true);
		}

		if(owner)
		{
			if(def_type==SchemaParser::SQL_DEFINITION)
			{
				attributes[ParsersAttributes::OWNER]=owner->getName(format);

				/** Only tablespaces and database do not have an ALTER OWNER SET
				 because the rule says that PostgreSQL tablespaces and database should be created
				 with just a command line isolated from the others **/
				if((def_type==SchemaParser::SQL_DEFINITION &&
						obj_type!=OBJ_TABLESPACE &&
						obj_type!=OBJ_DATABASE) ||
					 def_type==SchemaParser::XML_DEFINITION)
				{
					SchemaParser::setIgnoreUnkownAttributes(true);
					attributes[ParsersAttributes::OWNER]=
							SchemaParser::getCodeDefinition(ParsersAttributes::OWNER, attributes, def_type);
				}
			}
			else
				attributes[ParsersAttributes::OWNER]=owner->getCodeDefinition(def_type, true);
		}

		if(comment!="")
		{
			attributes[ParsersAttributes::COMMENT]=comment;

			if((def_type==SchemaParser::SQL_DEFINITION &&
					obj_type!=OBJ_TABLESPACE &&
					obj_type!=OBJ_DATABASE) ||
				 def_type==SchemaParser::XML_DEFINITION)
			{
				SchemaParser::setIgnoreUnkownAttributes(true);
				attributes[ParsersAttributes::COMMENT]=
						SchemaParser::getCodeDefinition(ParsersAttributes::COMMENT, attributes, def_type);
			}
		}

		if(reduced_form)
			attributes[ParsersAttributes::REDUCED_FORM]="1";
		else
			attributes[ParsersAttributes::REDUCED_FORM]="";

		try
		{
			code_def+=SchemaParser::getCodeDefinition(objs_schemas[obj_type], attributes, def_type);

			//Internally disabling the SQL definition
			if(sql_disabled && def_type==SchemaParser::SQL_DEFINITION)
			{
				//Creates a text stream and insert an comment start token on each line
				QTextStream ts(&code_def);
				QString buf;

				while(!ts.atEnd())
					buf+="-- " + ts.readLine() + "\n";

				//The entire commented buffer will be returned
				code_def=buf;
			}

			clearAttributes();
		}
		catch(Exception &e)
		{
			SchemaParser::restartParser();
			clearAttributes();

			if(e.getErrorType()==ERR_UNDEF_ATTRIB_VALUE)
				throw Exception(Exception::getErrorMessage(ERR_ASG_OBJ_INV_DEFINITION)
												.arg(Utf8String::create(this->getName(true)))
												.arg(this->getTypeName()),
												ERR_ASG_OBJ_INV_DEFINITION,__PRETTY_FUNCTION__,__FILE__,__LINE__,&e);
			else
				throw Exception(e.getErrorMessage(),e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__,&e);
		}
	}

	return(code_def);
}

void BaseObject::setAttribute(const QString &attrib, const QString &value)
{
	attributes[attrib]=value;
}

void BaseObject::clearAttributes(void)
{
	map<QString, QString>::iterator itr, itr_end;

	itr=attributes.begin();
	itr_end=attributes.end();

	while(itr!=itr_end)
	{
		itr->second="";
		itr++;
	}
}

void BaseObject::swapObjectsIds(BaseObject *obj1, BaseObject *obj2)
{
	if(!obj1 || !obj2)
		throw Exception(ERR_OPR_NOT_ALOC_OBJECT,__PRETTY_FUNCTION__,__FILE__,__LINE__);
	else if(obj1 != obj2)
	{
		unsigned id_bkp=obj1->object_id;
		obj1->object_id=obj2->object_id;
		obj2->object_id=id_bkp;
	}
}

vector<ObjectType> BaseObject::getObjectTypes(void)
{
	ObjectType types[]={ BASE_RELATIONSHIP, OBJ_AGGREGATE, OBJ_CAST, OBJ_COLLATION,
											 OBJ_COLUMN, OBJ_CONSTRAINT, OBJ_CONVERSION, OBJ_DATABASE,
											 OBJ_DOMAIN, OBJ_EXTENSION, OBJ_FUNCTION, OBJ_INDEX,
											 OBJ_LANGUAGE, OBJ_OPCLASS, OBJ_OPERATOR, OBJ_OPFAMILY,
											 OBJ_RELATIONSHIP, OBJ_ROLE, OBJ_RULE, OBJ_SCHEMA, OBJ_SEQUENCE,
											 OBJ_TABLE, OBJ_TABLESPACE, OBJ_TEXTBOX, OBJ_TRIGGER,
											 OBJ_TYPE, OBJ_VIEW, OBJ_PERMISSION };
	return(vector<ObjectType>(types, types + sizeof(types) / sizeof(ObjectType)));
}

void BaseObject::operator = (BaseObject &obj)
{
	this->owner=obj.owner;
	this->schema=obj.schema;
	this->tablespace=obj.tablespace;
	this->database=obj.database;
	this->comment=obj.comment;
	this->obj_name=obj.obj_name;
	this->obj_type=obj.obj_type;
	this->is_protected=obj.is_protected;
	this->sql_disabled=obj.sql_disabled;
	this->system_obj=obj.system_obj;
}

