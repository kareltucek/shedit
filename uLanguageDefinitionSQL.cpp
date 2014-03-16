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
  foreground = clBlack;
  number = (TColor)0x008888;
  comment =clGreen;
  commentback =(TColor)0xddffdd;
  string =clRed;
  variable =clGray;
  reserved =clBlue;
  preprocessor =(TColor)0x55ff55;

  SetDefaultColor(&foreground);
  SetCaseSensitive(false);


  AddKeywords(L"ACCESS ADD ALL  ALTER  AND  ANY  AS  ASC AUDIT BETWEEN  BY  CHAR  CHECK  CLUSTER COLUMN  COLUMN_VALUE COMMENT COMPRESS CONNECT  CREATE  CURRENT  DATE  DECIMAL  DEFAULT  DELETE  DESC DISTINCT  DROP  ELSE  EXCLUSIVE EXISTS  FILE FLOAT  FOR  FROM  GRANT  GROUP  HAVING  IDENTIFIED IMMEDIATE IN  INCREMENT INDEX INITIAL INSERT INTEGER  INTERSECT  INTO  IS  LEVEL LIKE  LOCK LONG MAXEXTENTS MINUS MLSLABEL MODE MODIFY NESTED_TABLE_ID NOAUDIT NOCOMPRESS NOT  NOWAIT NULL  NUMBER OF  OFFLINE ON  ONLINE OPTION OR  ORDER  PCTFREE PRIOR PRIVILEGES PUBLIC RAW RENAME RESOURCE REVOKE  ROW  ROWID ROWNUM ROWS  SELECT  SESSION SET  SHARE SIZE SMALLINT  START  SUCCESSFUL SYNONYM SYSDATE TABLE  THEN  TO  TRIGGER  UID UNION  UNIQUE  UPDATE  USER  VALIDATE VALUES  VARCHAR  VARCHAR2 VIEW WHENEVER  WHERE  WITH", new FontStyle(&reserved, NULL, TFontStyles() << fsBold));



  AddKeywords(L"1 2 3 4 5 6 7 8 9 0", new FontStyle(&number));

  FontStyle * commentstyle = new FontStyle(&comment, &commentback);
  TreeNode * commentTree = AddPair(L"/*", L"*/", commentstyle);
  AddJump(L"--", commentstyle, LangDefSpecType::LineTag, NULL, commentTree);
  AddJump(L"prompt", commentstyle, LangDefSpecType::LineTag, NULL, commentTree);

  AddKeywords(L"TODO", new FontStyle(&foreground), commentTree);

  AddWord(L":", new FontStyle(&variable, NULL, TFontStyles()));


  FontStyle * stringstyle = new FontStyle(&string);
  AddPair(L"'", L"'", stringstyle);
  AddPair(L"\"", L"\"", stringstyle);

}
