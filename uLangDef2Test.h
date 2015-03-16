LanguageDefinition::LanguageDefinition()
{
  int i = 1;
  AddTerm("IDENTIF", NULL, "[a-zA-Z][a-zA-Z0-9_-]*", ++i, 0);
  AddTerm("TYPE", NULL, "(int|bool|char|string)", ++i, 0);
  AddTerm("OP", NULL, "[+*/-]", ++i, 0);
  AddTerm("ASS", NULL, "=", ++i, 0);
  AddTerm("NUM", NULL, "[0-9]+", ++i, 0);
  AddTerm("SEMIC", NULL, ";", ++i, 0);
  AddTerm("WHITE", NULL, "[ \t\n\r]+", ++i, 0);

  AddNonTerm("program", NULL, "decl|ass", ++i, 0);
  AddNonTerm("decl", NULL, "TYPE IDENT SEMIC", ++i, 0);
  AddNonTerm("oper", NULL, "num OP oper", ++i, 0);
  AddNonTerm("ass", NULL, "IDENTIF ASS oper SEMIC | IDENTIF ASS IDENTIF SEMIC", ++i, 0);
}
