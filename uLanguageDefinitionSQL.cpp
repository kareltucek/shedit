//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition.h"
#include "uLanguageDefinitionSQL.h"
#include "uFormat.h"
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
  TColor * number = new TColor((TColor)0x008888);
  TColor * comment = new TColor(clGreen);
  TColor * commentback = new TColor((TColor)0xddffdd);
  TColor * string = new TColor(clRed);
  TColor * variable = new TColor(clRed);
  TColor * reserved = new TColor(clBlue);
  TColor * preprocessor = new TColor((TColor)0x55ff55);

  SetDefaultColor(foreground);
  SetCaseSensitive(false);


  AddReservedNames(L"ACCESS ADD ALL  ALTER  AND  ANY  AS  ASC AUDIT BETWEEN  BY  CHAR  CHECK  CLUSTER COLUMN  COLUMN_VALUE COMMENT COMPRESS CONNECT  CREATE  CURRENT  DATE  DECIMAL  DEFAULT  DELETE  DESC DISTINCT  DROP  ELSE  EXCLUSIVE EXISTS  FILE FLOAT  FOR  FROM  GRANT  GROUP  HAVING  IDENTIFIED IMMEDIATE IN  INCREMENT INDEX INITIAL INSERT  INTEGER  INTERSECT  INTO  IS  LEVEL LIKE  LOCK LONG MAXEXTENTS MINUS MLSLABEL MODE MODIFY NESTED_TABLE_ID NOAUDIT NOCOMPRESS NOT  NOWAIT NULL  NUMBER OF  OFFLINE ON  ONLINE OPTION OR  ORDER  PCTFREE PRIOR PRIVILEGES PUBLIC RAW RENAME RESOURCE REVOKE  ROW  ROWID ROWNUM ROWS  SELECT  SESSION SET  SHARE SIZE SMALLINT  START  SUCCESSFUL SYNONYM SYSDATE TABLE  THEN  TO  TRIGGER  UID UNION  UNIQUE  UPDATE  USER  VALIDATE VALUES  VARCHAR  VARCHAR2 VIEW WHENEVER  WHERE  WITH", new FontStyle(reserved));
  AddReservedNames(L"1 2 3 4 5 6 7 8 9 0", new FontStyle(number));
  //AddSpecial(L"--", L"", LangDefSpecType::LineTag, comment);
  FontStyle * commentstyle = new FontStyle(comment, commentback);
  TreeItem * commentTree = AddPair(L"/*", L"*/", commentstyle);
  AddJump(L"//", commentstyle, LangDefSpecType::LineTag, NULL, commentTree);
  AddReservedNames(L"TODO", new FontStyle(foreground), commentTree);

  AddLine(L"#", new FontStyle(preprocessor));
  AddWord(L"$", new FontStyle(variable));


  FontStyle * stringstyle = new FontStyle(string);
  AddPair(L"'", L"'", stringstyle);
  AddPair(L"\"", L"\"", stringstyle);

}

