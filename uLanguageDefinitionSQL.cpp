//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition.h"
#include "uLanguageDefinitionSQL.h"
#include <string>
#include <stdio.h>

using namespace SHEdit;

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
  LanguageDefinitionSQL::LanguageDefinitionSQL()
:  LanguageDefinition()
{
  TColor * foreground = new TColor(clBlack);
  TColor * number = new TColor(clYellow);
  TColor * comment = new TColor(clGreen);
  TColor * string = new TColor(clRed);
  TColor * reserved = new TColor(clBlue);

  SetDefaultColor(foreground);
  SetCaseSensitive(false);
  AddReservedNames(L"ACCESS ADD ALL  ALTER  AND  ANY  AS  ASC AUDIT BETWEEN  BY  CHAR  CHECK  CLUSTER COLUMN  COLUMN_VALUE COMMENT COMPRESS CONNECT  CREATE  CURRENT  DATE  DECIMAL  DEFAULT  DELETE  DESC DISTINCT  DROP  ELSE  EXCLUSIVE EXISTS  FILE FLOAT  FOR  FROM  GRANT  GROUP  HAVING  IDENTIFIED IMMEDIATE IN  INCREMENT INDEX INITIAL INSERT  INTEGER  INTERSECT  INTO  IS  LEVEL LIKE  LOCK LONG MAXEXTENTS MINUS MLSLABEL MODE MODIFY NESTED_TABLE_ID NOAUDIT NOCOMPRESS NOT  NOWAIT NULL  NUMBER OF  OFFLINE ON  ONLINE OPTION OR  ORDER  PCTFREE PRIOR PRIVILEGES PUBLIC RAW RENAME RESOURCE REVOKE  ROW  ROWID (See Note 2 at the end of this list) ROWNUM ROWS  SELECT  SESSION SET  SHARE SIZE SMALLINT  START  SUCCESSFUL SYNONYM SYSDATE TABLE  THEN  TO  TRIGGER  UID UNION  UNIQUE  UPDATE  USER  VALIDATE VALUES  VARCHAR  VARCHAR2 VIEW WHENEVER  WHERE  WITH", reserved);
  //AddReservedNames(L"1 2 3 4 5 6 7 8 9 0", number);
  //AddSpecial(L"--", L"", LangDefSpecType::LineTag, comment);
  //AddSpecial(L"prompt", L"", LangDefSpecType::LineTag, comment);
  //AddSpecial(L"/*", L"*/", LangDefSpecType::PairTag, comment);
  //AddSpecial(L"'", L"'", LangDefSpecType::PairTag, string);

}

