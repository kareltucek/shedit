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
  background = clWhite;
  number = (TColor)0x800000;
  comment = (TColor)0x008000;
  //commentback =(TColor)0xddffdd;
  commentback =(TColor)0xffffff;
  string = (TColor)0x000080;
  variable = (TColor)0x0af6ce3;
  reserved =clBlack;

  SetDefaultColor(&foreground);
  SetCaseSensitive(false);

  AddKeywords(L" ABORT ACCEPT ACCESS ADD ADMIN AFTER ALL ALLOCATE ALTER ANALYZE AND ANY ARCHIVE ARCHIVELOG ARRAY ARRAYLEN AS ASC ASSERT ASSIGN AT AUDIT AUTHORIZATION AVG BACKUP BASE_TABLE BECOME BEFORE BEGIN BETWEEN BINARY_INTEGER BLOCK BODY BOOLEAN BY CACHE CANCEL CASCADE CASE CHANGE CHAR CHARACTER CHAR_BASE CHECK CHECKPOINT CLOSE CLUSTER CLUSTERS COBOL COLAUTH COLUMN COLUMNS COMMENT COMMIT COMPILE COMPRESS CONNECT CONSTANT CONSTRAINT CONSTRAINTS CONTENTS CONTINUE CONTROLFILE COUNT CRASH CREATE CURRENT CURRVAL CURSOR CYCLE DATABASE DATA_BASE DATAFILE DATE DBA _DEBUGOFF _DEBUGON DEC DECIMAL DECLARE DEFAULT DEFINITION DELAY DELETE DELTA DESC DIGITS DISABLE DISMOUNT DISPOSE DISTINCT DO DOUBLE DROP DUMP EACH ELSE ELSIF ENABLE END ENTRY ESCAPE EVENTS EXCEPT EXCEPTION EXCEPTION_INIT EXCEPTIONS EXCLUSIVE EXEC EXECUTE EXISTS EXIT EXPLAIN EXTENT EXTERNALLY FALSE FETCH FILE FLOAT FLUSH FOR FORCE FOREIGN FORM FORTRAN FOUND FREELIST FREELISTS FROM FUNCTION GO GOTO GRANT GROUP GROUPS HAVING IDENTIFIED IMMEDIATE IN INCLUDING INCREMENT INDEX INDICATOR INITIAL INITRANS INSERT INSTANCE INT INTEGER INTERSECT INTO IS KEY LANGUAGE LAYER LEVEL LIKE LINK LISTS LOCK LOGFILE LONG MANAGE MANUAL MAX MAXDATAFILES MAXEXTENTS MAXINSTANCES MAXLOGFILES MAXLOGHISTORY MAXLOGMEMBERS MAXTRANS MAXVALUE MIN MINEXTENTS MINUS MINVALUE MODE MODIFY MODULE MOUNT NEW NEXT NOARCHIVELOG NOAUDIT NOCACHE NOCOMPRESS NOCYCLE NOMAXVALUE NOMINVALUE NONE NOORDER NORESETLOGS NORMAL NOSORT NOT NOTFOUND NOWAIT NULL NUMBER NUMERIC OF OFF OFFLINE OLD ON ONLINE ONLY OPEN OPTIMAL OPTION OR ORDER OWN PACKAGE PARALLEL PCTFREE PCTINCREASE PCTUSED PLAN PLI PRECISION PRIMARY PRIOR PRIVATE PRIVILEGES PROCEDURE PROFILE PUBLIC QUOTA RAW READ REAL RECOVER REFERENCES REFERENCING RENAME RESETLOGS RESOURCE RESTRICTED REUSE REVOKE ROLE ROLES ROLLBACK ROW ROWID ROWLABEL ROWNUM ROWS SAVEPOINT SCHEMA SCN SECTION SEGMENT SELECT SEQUENCE SESSION SET SHARE SHARED SIZE SMALLINT SNAPSHOT SOME SORT SQL SQLBUF SQLCODE SQLERROR SQLSTATE START STATEMENT_ID STATISTICS STOP STORAGE SUCCESSFUL SUM SWITCH SYNONYM SYSDATE SYSTEM TABLE TABLES TABLESPACE TEMPORARY THEN THREAD TIME TO TRACING TRANSACTION TRIGGER TRIGGERS TRUNCATE UID UNDER UNION UNIQUE UNLIMITED UNTIL UPDATE USE USER USING VALIDATE VALUES VARCHAR VARCHAR2 VIEW WHEN WHENEVER WHERE WITH WORK WRITE " , new FontStyle(&reserved, NULL , TFontStyles() << fsBold ));


  AddKeywords(L"1 2 3 4 5 6 7 8 9 0 1. 2. 3. 4. 5. 6. 7. 8. 9.", new FontStyle(&number));

  FontStyle * commentstyle = new FontStyle(&comment, &commentback);
  TreeNode * commentTree = AddPair(L"/*", L"*/", commentstyle);
  TreeNode * commentTree2 = AddLine(L"--", commentstyle);
  //AddLine(L"prompt", commentstyle, NULL, commentTree2);

  AddKeywords(L"TODO", new FontStyle(&foreground, &background), commentTree);

  AddWord(L":", new FontStyle(&variable, &background, TFontStyles()));


  FontStyle * stringstyle = new FontStyle(&string, &background);
  AddPair(L"'", L"'", stringstyle);
  AddPair(L"\"", L"\"", stringstyle);

}
