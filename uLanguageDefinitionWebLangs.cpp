//---------------------------------------------------------------------------
#pragma hdrstop


#include "uLanguageDefinition.h"
#include "uLanguageDefinitionWebLangs.h"
#include "uFormat.h"
#include <string>
#include <stdio.h>
#include <vcl.h>

using namespace SHEdit;

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
  LanguageDefinitionWebLangs::LanguageDefinitionWebLangs()
:  LanguageDefinition()
{

  htmlBG = clWhite;
  htmlFG = clBlack;
  htmlAttribs = clBlue;
  htmlComments = clGreen;
  htmlTags = (TColor)0xAA00AA;  //violet
  htmlQuotes = clGreen;

  phpFG = clBlue;
  phpBG = (TColor)0xFFF0F0;    //blue
  phpVariable = (TColor)0x0000CC;
  phpQuotes = clRed;
  phpQuotesBack = phpBG;
  phpComments = clGreen;

  cssBG = (TColor)0xF0FFF0;        //green
  cssFG = htmlFG;
  cssAttr = (TColor)0x00AA00;
  cssVal = clBlue;

  jvFG = htmlFG;
  jvBG = (TColor)0xAAFFFF; //yellow
  jvComments = clGreen;
  jvQuotes = clRed;
  jvKeywords = clBlue;

  SetDefaultColor(&htmlFG);
  SetCaseSensitive(true);

  //html
  htmlTree = GetTree();
  SetTreeCaseSensitive(htmlTree, false);
  htmlCommentTree = NULL;
  htmlTagTree = NULL;

  ConstructHtml(htmlTree, &htmlBG, &htmlFG);

  phpTree = AddPair(L"<?", L"?>", new FontStyle(&phpFG,&phpBG), htmlTree, NULL);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, htmlCommentTree, phpTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, htmlTagTree, phpTree);

  //php
  phpCommentTree = NULL;
  ConstructPhp(phpTree, &phpBG, &phpFG);
  AddJump(L"?>", new FontStyle(&phpComments, &phpBG), LangDefSpecType::Jump, phpCommentTree, htmlTree);

  //echo html parsing
  phpHtmlEnterTree = AddDupTree(phpTree, NULL);
  phpHtmlEnterTree->map[(int)'"'] = NULL;
  phpHtmlEnterTree->map[(int)'\''] = NULL;
  AddPush(L"echo", NULL, phpTree, phpHtmlEnterTree, PUSH_ALWAYS, MASK_ECHO_MODE);
  AddPush(L"[", NULL, phpHtmlEnterTree, phpTree, MASK_ECHO_MODE, MASK_ECHO_IGNORE | MASK_ECHO_MODE);
  AddPop(L"]", NULL, phpTree, MASK_ECHO_IGNORE, POP_AUTO);
  AddPushFront(L"'", new FontStyle(&phpQuotes, &phpBG), phpHtmlEnterTree, htmlTree, MASK_ECHO_MODE, MASK_PHP_APOSTROPHE | MASK_PREDICTION);
  AddPushFront(L"\"", new FontStyle(&phpQuotes, &phpBG), phpHtmlEnterTree, htmlTree, MASK_ECHO_MODE, MASK_PHP_QUOTE | MASK_PREDICTION);
  AddPop(L";", new FontStyle(), phpHtmlEnterTree, MASK_ECHO_MODE, POP_AUTO);
  AddJumpFront(L"?>", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, phpTree, htmlTree, 0, MASK_PREDICTION);

  //css
  cssPropertyTree = NULL;
  cssValueTree = NULL;
  cssEnterTree = AddDupTree(htmlTagTree, new FontStyle(&cssFG, &cssBG));
  cssTree = AddNewTree( new FontStyle(&cssFG, &cssBG));
  ConstructCss(cssTree, &cssBG, &cssFG);
  //AddJump(L"style", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, tagTree, cssEnterTree);
  AddJump(L"<style", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, htmlTree, cssEnterTree);
  AddPush(L"style='", new FontStyle(&htmlAttribs, &cssBG), htmlTagTree, cssPropertyTree, PUSH_ALWAYS, MASK_APOSTROPHE);
  AddPop(L"style='", NULL, htmlTagTree, MASK_APOSTROPHE, POP_AUTO);
  AddPush(L"style=\"", new FontStyle(&htmlAttribs, &cssBG), htmlTagTree, cssPropertyTree, PUSH_ALWAYS, MASK_QUOTE);
  AddPop(L"style=\"", NULL, htmlTagTree, MASK_QUOTE, POP_AUTO);
  AddPush(L"style=\\\"", new FontStyle(&htmlAttribs, &cssBG), htmlTagTree, cssPropertyTree, PUSH_ALWAYS, MASK_DOUBLE_QUOTE);
  AddPop(L"style=\\\"", NULL, htmlTagTree, MASK_DOUBLE_QUOTE, POP_AUTO);
  cssEnterTree->map[(int)'>'] = NULL;
  AddJump(L">", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, cssEnterTree, cssTree);
  AddJump(L"</style", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, cssTree, htmlTagTree);
  AddJump(L"</style>", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, cssTree, htmlTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, cssTree, phpTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, cssEnterTree, phpTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, cssValueTree, phpTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, cssPropertyTree, phpTree);
  AddPops(cssEnterTree);

  //javascript
  jvTree = AddNewTree(new FontStyle(&jvFG, &jvBG));
  jvEnterTree = AddDupTree(htmlTagTree, new FontStyle(&htmlFG, &htmlBG));
  ConstructJavascript(jvTree, &jvBG, &jvFG);
  AddJump(L"<script", new FontStyle(&htmlTags, &htmlBG), LangDefSpecType::Jump, htmlTree, jvEnterTree);
  jvEnterTree->map[(int)'>'] = NULL;
  AddJump(L">", new FontStyle(&htmlTags, &htmlBG), LangDefSpecType::Jump, jvEnterTree, jvTree);
  AddJump(L"</script>", new FontStyle(&htmlTags, &htmlBG), LangDefSpecType::Jump, jvTree, htmlTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, jvTree, phpTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, jvEnterTree, phpTree);
  AddJump(L"<?", new FontStyle(&phpFG,&phpBG), LangDefSpecType::Jump, jvCommentTree, phpTree);
  AddPops(jvEnterTree);

  //inline javascript
  AddPushes(L" onafterprint=' onbeforeprint=' onbeforeunload=' onerror=' onhaschange=' onload=' onmessage=' onoffline=' ononline=' onpagehide=' onpageshow=' onpopstate=' onredo=' onresize=' onstorage=' onundo=' onunload=' onblur=' onchange=' oncontextmenu=' onfocus=' onformchange=' onforminput=' oninput=' oninvalid=' onreset=' onselect=' onsubmit=' onkeydown=' onkeypress=' onkeyup=' onabort=' oncanplay=' oncanplaythrough=' ondurationchange=' onemptied=' o    nended=' onerror=' onloadeddata=' onloadedmetadata=' onloadstart=' onpause=' onplay=' onplaying=' onprogress=' onratechange=' onreadystatechange=' onseeked=' onseeking=' onstalled=' onsuspend=' ontimeupdate=' onvolumechange=' onwaiting=' onclick=' ondblclick=' ondrag=' ondragend=' ondragenter=' ondragleave=' ondragover=' ondragstart=' ondrop=' onmousedown=' onmousemove=' onmouseout=' onmouseover=' onmouseup=' onmousewheel=' onscroll='"
            , new FontStyle(&htmlAttribs, &htmlBG), htmlTagTree, jvTree, PUSH_ALWAYS, MASK_APOSTROPHE);
  LanguageDefinition::AddPops(L" onafterprint=' onbeforeprint=' onbeforeunload=' onerror=' onhaschange=' onload=' onmessage=' onoffline=' ononline=' onpagehide=' onpageshow=' onpopstate=' onredo=' onresize=' onstorage=' onundo=' onunload=' onblur=' onchange=' oncontextmenu=' onfocus=' onformchange=' onforminput=' oninput=' oninvalid=' onreset=' onselect=' onsubmit=' onkeydown=' onkeypress=' onkeyup=' onabort=' oncanplay=' oncanplaythrough=' ondurationchange=' onemptied=' o    nended=' onerror=' onloadeddata=' onloadedmetadata=' onloadstart=' onpause=' onplay=' onplaying=' onprogress=' onratechange=' onreadystatechange=' onseeked=' onseeking=' onstalled=' onsuspend=' ontimeupdate=' onvolumechange=' onwaiting=' onclick=' ondblclick=' ondrag=' ondragend=' ondragenter=' ondragleave=' ondragover=' ondragstart=' ondrop=' onmousedown=' onmousemove=' onmouseout=' onmouseover=' onmouseup=' onmousewheel=' onscroll='"
            , NULL, htmlTagTree, MASK_APOSTROPHE, POP_AUTO);
   AddPushes(L"onafterprint=\" onbeforeprint=\" onbeforeunload=\" onerror=\" onhaschange=\" onload=\" onmessage=\" onoffline=\" ononline=\" onpagehide=\" onpageshow=\" onpopstate=\" onredo=\" onresize=\" onstorage=\" onundo=\" onunload=\" onblur=\" onchange=\" oncontextmenu=\" onfocus=\" onformchange=\" onforminput=\" oninput=\" oninvalid=\" onreset=\" onselect=\" onsubmit=\" onkeydown=\" onkeypress=\" onkeyup=\" onabort=\" oncanplay=\" oncanplaythrough=\" ondurationchange=\" onemptied=\" onended=\" onerror=\" onloadeddata=\" onloadedmetadata=\" onloadstart=\" onpause=\" onplay=\" onplaying=\" onprogress=\" onratechange=\" onreadystatechange=\" onseeked=\" onseeking=\" onstalled=\" onsuspend=\" ontimeupdate=\" onvolumechange=\" onwaiting=\" onclick=\" ondblclick=\" ondrag=\" ondragend=\" ondragenter=\" ondragleave=\" ondragover=\" ondragstart=\" ondrop=\" onmousedown=\" onmousemove=\" onmouseout=\" onmouseover=\" onmouseup=\" onmousewheel=\" onscroll=\""
            , new FontStyle(&htmlAttribs, &htmlBG), htmlTagTree, jvTree, PUSH_ALWAYS, MASK_QUOTE);
   LanguageDefinition::AddPops(L"onafterprint=\" onbeforeprint=\" onbeforeunload=\" onerror=\" onhaschange=\" onload=\" onmessage=\" onoffline=\" ononline=\" onpagehide=\" onpageshow=\" onpopstate=\" onredo=\" onresize=\" onstorage=\" onundo=\" onunload=\" onblur=\" onchange=\" oncontextmenu=\" onfocus=\" onformchange=\" onforminput=\" oninput=\" oninvalid=\" onreset=\" onselect=\" onsubmit=\" onkeydown=\" onkeypress=\" onkeyup=\" onabort=\" oncanplay=\" oncanplaythrough=\" ondurationchange=\" onemptied=\" onended=\" onerror=\" onloadeddata=\" onloadedmetadata=\" onloadstart=\" onpause=\" onplay=\" onplaying=\" onprogress=\" onratechange=\" onreadystatechange=\" onseeked=\" onseeking=\" onstalled=\" onsuspend=\" ontimeupdate=\" onvolumechange=\" onwaiting=\" onclick=\" ondblclick=\" ondrag=\" ondragend=\" ondragenter=\" ondragleave=\" ondragover=\" ondragstart=\" ondrop=\" onmousedown=\" onmousemove=\" onmouseout=\" onmouseover=\" onmouseup=\" onmousewheel=\" onscroll=\""
            , NULL, htmlTagTree, MASK_QUOTE,POP_AUTO);
    AddPushes(L"onafterprint=\\\" onbeforeprint=\\\" onbeforeunload=\\\" onerror=\\\" onhaschange=\\\" onload=\\\" onmessage=\\\" onoffline=\\\" ononline=\\\" onpagehide=\\\" onpageshow=\\\" onpopstate=\\\" onredo=\\\" onresize=\\\" onstorage=\\\" onundo=\\\" onunload=\\\" onblur=\\\" onchange=\\\" oncontextmenu=\\\" onfocus=\\\" onformchange=\\\" onforminput=\\\" oninput=\\\" oninvalid=\\\" onreset=\\\" onselect=\\\" onsubmit=\\\" onkeydown=\\\" onkeypress=\\\" onkeyup=\\\" onabort=\\\" oncanplay=\\\" oncanplaythrough=\\\" ondurationchange=\\\" onemptied=\\\" onended=\\\" onerror=\\\" onloadeddata=\\\" onloadedmetadata=\\\" onloadstart=\\\" onpause=\\\" onplay=\\\" onplaying=\\\" onprogress=\\\" onratechange=\\\" onreadystatechange=\\\" onseeked=\\\" onseeking=\\\" onstalled=\\\" onsuspend=\\\" ontimeupdate=\\\" onvolumechange=\\\" onwaiting=\\\" onclick=\\\" ondblclick=\\\" ondrag=\\\" ondragend=\\\" ondragenter=\\\" ondragleave=\\\" ondragover=\\\" ondragstart=\\\" ondrop=\\\" onmousedown=\\\" onmousemove=\\\" onmouseout=\\\" onmouseover=\\\" onmouseup=\\\" onmousewheel=\\\" onscroll=\\\""
             , new FontStyle(&htmlAttribs, &htmlBG), htmlTagTree, jvTree, PUSH_ALWAYS, MASK_DOUBLE_QUOTE);
    LanguageDefinition::AddPops(L"onafterprint=\\\" onbeforeprint=\\\" onbeforeunload=\\\" onerror=\\\" onhaschange=\\\" onload=\\\" onmessage=\\\" onoffline=\\\" ononline=\\\" onpagehide=\\\" onpageshow=\\\" onpopstate=\\\" onredo=\\\" onresize=\\\" onstorage=\\\" onundo=\\\" onunload=\\\" onblur=\\\" onchange=\\\" oncontextmenu=\\\" onfocus=\\\" onformchange=\\\" onforminput=\\\" oninput=\\\" oninvalid=\\\" onreset=\\\" onselect=\\\" onsubmit=\\\" onkeydown=\\\" onkeypress=\\\" onkeyup=\\\" onabort=\\\" oncanplay=\\\" oncanplaythrough=\\\" ondurationchange=\\\" onemptied=\\\" onended=\\\" onerror=\\\" onloadeddata=\\\" onloadedmetadata=\\\" onloadstart=\\\" onpause=\\\" onplay=\\\" onplaying=\\\" onprogress=\\\" onratechange=\\\" onreadystatechange=\\\" onseeked=\\\" onseeking=\\\" onstalled=\\\" onsuspend=\\\" ontimeupdate=\\\" onvolumechange=\\\" onwaiting=\\\" onclick=\\\" ondblclick=\\\" ondrag=\\\" ondragend=\\\" ondragenter=\\\" ondragleave=\\\" ondragover=\\\" ondragstart=\\\" ondrop=\\\" onmousedown=\\\" onmousemove=\\\" onmouseout=\\\" onmouseover=\\\" onmouseup=\\\" onmousewheel=\\\" onscroll=\\\""
            , NULL, htmlTagTree, MASK_DOUBLE_QUOTE,POP_AUTO);

  //AddSafetyJumps();



#ifdef DEBUG
  htmlTree->Name = L"html";
  htmlCommentTree->Name = L"html comment";
  htmlTagTree->Name = L"html tag tree";
  phpTree->Name = L"php";
  phpCommentTree->Name = L"php comment";
  cssPropertyTree->Name = L"css property";
  cssValueTree->Name = L"css value";
  cssEnterTree->Name = L"css entertree";
  cssTree->Name = L"css";
  phpHtmlEnterTree->Name = L"php html enter tree";
#endif

  GetNewBankID();
  SetBankBase(0, htmlTree);
  SetBankMask(0, MASK_APOSTROPHE | MASK_QUOTE | MASK_DOUBLE_QUOTE | MASK_PREDICTION);
  htmlTree->bankID = 0;
  htmlCommentTree->bankID = 0;
  htmlTagTree->bankID = 0;
  htmlQuoteTree->bankID = 0;

  cssPropertyTree->bankID = 0;
  cssValueTree->bankID = 0;
  cssEnterTree->bankID = 0;
  cssTree->bankID = 0;
  cssQuoteTree->bankID = 0;

  jvTree->bankID = 0;
  jvEnterTree->bankID = 0;
  jvCommentTree->bankID = 0;
  jvQuoteTree->bankID = 0;

  SetBankBase(1, phpTree);
  SetBankMask(1, MASK_PHP | MASK_ECHO_MODE | MASK_ECHO_IGNORE | MASK_PHP_APOSTROPHE | MASK_PHP_QUOTE);
  phpTree->bankID = 1;
  phpCommentTree->bankID = 1;
  phpHtmlEnterTree->bankID = 1;
  phpQuoteTree->bankID = 1;
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructHtml(LanguageDefinition::TreeNode * at, TColor * bg, TColor * fg)
{
  htmlTagTree = AddPair(L"<", L">", new FontStyle(&htmlTags, bg), at);
  htmlTagTree->format = new FontStyle(fg, bg);
  AddKeywords(L"a abbr acronym address applet area article aside audio b base basefont bdi bdo bgsound big blink blockquote body br button canvas caption center cite code col colgroup content data datalist dd decorator del details dfn dir div dl dt element em embed fieldset figcaption figure font footer form frame frameset h1 h2 h3 h4 h5 h6 head header hgroup hr html i iframe img input ins isindex kbd keygen label legend li link listing main map mark marquee menu menuitem meta meter nav nobr noframes noscript object ol optgroup option output p param plaintext pre progress q rp rt ruby s samp script section select shadow small source spacer span strike strong style sub summary sup table tbody td template textarea tfoot th thead time title tr track tt u ul var video wbr xmp"
                    , new FontStyle(&htmlTags, NULL), htmlTagTree);
  AddKeywords(L"accept accept-charset accesskey action align alt async autocomplete autofocus autoplay bgcolor buffered challenge charset checked cite class code codebase color cols colspan content contenteditable contextmenu controls coords data datetime default defer dir dirname disabled download draggable dropzone enctype for form headers height hidden high href hreflang http-equiv icon id ismap itemprop keytype kind label lang language list loop low manifest max maxlength media method min multiple name novalidate open optimum pattern ping placeholder poster preload pubdate radiogroup readonly rel required reversed rows rowspan sandbox spellcheck scope scoped seamless selected shape size sizes span src srcdoc srclang start step style summary tabindex target title type usemap value width wrap "
                    , new FontStyle(&htmlAttribs, NULL), htmlTagTree);
  htmlQuoteTree = AddPushPopPair(L"\"", L"\"", new FontStyle(&htmlQuotes, bg), htmlTagTree, NULL, MASK_QUOTE);
  AddPushPopPair(L"\\\"", L"\\\"", new FontStyle(&htmlQuotes, bg), htmlTagTree, htmlQuoteTree, MASK_DOUBLE_QUOTE);
  AddPushPopPair(L"'", L"'", new FontStyle(&htmlQuotes, bg), htmlTagTree, htmlQuoteTree, MASK_APOSTROPHE);
  //htmlQuoteTree->Name = L"html quotes";

  htmlCommentTree = AddPair(L"<!--", L"-->", new FontStyle(&htmlComments, bg));

  AddPops(htmlCommentTree);
  AddPops(at);
  AddPops(htmlTagTree);
  AddPops(htmlQuoteTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructCss(LanguageDefinition::TreeNode * at, TColor * bg, TColor * fg)
{
  FontStyle * tagStyle = new FontStyle(&htmlTags, bg);
  AddKeywords(L"a abbr acronym address applet area article aside audio b base basefont bdi bdo bgsound big blink blockquote body br button canvas caption center cite code col colgroup content data datalist dd decorator del details dfn dir div dl dt element em embed fieldset figcaption figure font footer form frame frameset h1 h2 h3 h4 h5 h6 head header hgroup hr html i iframe img input ins isindex kbd keygen label legend li link listing main map mark marquee menu menuitem meta meter nav nobr noframes noscript object ol optgroup option output p param plaintext pre progress q rp rt ruby s samp script section select shadow small source spacer span strike strong style sub summary sup table tbody td template textarea tfoot th thead time title tr track tt u ul var video wbr xmp"
                  , tagStyle, at);
  AddWord(L".", tagStyle, at);
  AddWord(L"#", tagStyle, at);
  AddWord(L":", tagStyle, at);

  cssPropertyTree = AddPair(L"{", L"}", at->format, at);
  AddKeywords( L"azimuth background-attachment background-color background-image background-position background-repeat background border-collapse border-color border-spacing border-style border-top border-right border-bottom border-left border-top-color border-right-color border-bottom-color border-left-color border-top-style border-right-style border-bottom-style border-left-style border-top-width border-right-width border-bottom-width border-left-width border-width border bottom caption-side clear clip color content counter-increment counter-reset cue-after cue-before cue cursor direction display elevationempty-cells float font-family font-size font-style font-variant font-weight font height left letter-spacing line-height list-style-image list-style-position list-style-type list-style margin-right margin-left margin-top margin-bottom margin max-height max-width min-height min-width orphans outline-color outline-style outline-width outline overflow padding-top padding-right padding-bottom padding-left padding page-break-after page-break-before page-break-inside pause-after pause-before pause pitch-range pitch play-during position quotes richness right speak-header speak-numeral speak-punctuation speak speech-rate stress table-layout text-align text-decoration text-indent text-transform top unicode-bidi vertical-align visibility voice-family volume white-space widows width word-spacing z-index"
                , new FontStyle(&cssAttr, bg), cssPropertyTree);

  FontStyle * defstyle = new FontStyle(fg, bg);
  cssValueTree = AddNewTree(defstyle);
  AddJump(L"}", defstyle, LangDefSpecType::Jump, cssValueTree, at);
  AddJump(L":", defstyle, LangDefSpecType::Jump, cssPropertyTree, cssValueTree);
  AddJump(L";", defstyle, LangDefSpecType::Jump, cssValueTree, cssPropertyTree);


  FontStyle * valstyle = new FontStyle(&cssVal, bg);
  AddKeywords(L"large px % rgb rgba src url 0 1 2 3 4 5 6 7 8 9 em en  none hidden dotted dashed solid double groove ridge inset outset initial above absolute always armenian auto avoid baseline behind below bidi-override blink block bold bolder both bottom capitalize caption center center-left center-right circle close-quote code collapse continuous crosshair 'cue-after' cue-before' decimal decimal-leading-zero default digits disc embed e-resize far-left far-right fast faster fixed georgian help hidden hide high higher icon inherit inline inline-block inline-table inside italic justify left left-side leftwards level lighter line-through list-item loud low lower lower-alpha lowercase lower-greek lower-latin lower-roman ltr medium menu message-box middle mix move ne-resize no-close-quote none no-open-quote no-repeat normal nowrap n-resize nw-resize oblique once open-quote outside overline pointer pre pre-line pre-wrap progress relative repeat repeat-x repeat-y right right-side rightwards rtl scroll separate se-resize show silent slow slower small small-caps small-caption soft spell-out square s-resize static status-bar sub super sw-resize table table-caption table-cell table-column table-column-group table-footer-group table-header-group table-row table-row-group text text-bottom text-top top transparentnone underline upper-alpha uppercase upper-latin upper-roman visible visible wait w-resize x-fast x-high x-loud x-low x-slow x-soft"
                , valstyle, cssValueTree);
  AddWord(L"#", valstyle, cssValueTree);

  cssQuoteTree = AddPushPopPair(L"\"", L"\"", valstyle, cssValueTree, NULL, MASK_QUOTE);
  AddPushPopPair(L"\\\"", L"\\\"", valstyle, cssValueTree, cssQuoteTree, MASK_DOUBLE_QUOTE);
  AddPushPopPair(L"'", L"'", valstyle, cssValueTree, cssQuoteTree, MASK_APOSTROPHE);
  //cssQuoteTree->Name = L"css quotes";

  AddPops(at);
  AddPops(cssPropertyTree);
  AddPops(cssValueTree);
  AddPops(cssQuoteTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructPhp(LanguageDefinition::TreeNode * at, TColor * bg, TColor * fg)
{
  AddKeywords(L"__halt_compiler abstract and array as break callable case catch class clone const continue declare default die do echo else elseif empty enddeclare endfor endforeach endif endswitch endwhile eval exit extends final for foreach function global goto if implements include include_once instanceof insteadof interface isset list namespace new or print private protected public require require_once return static switch throw trait try unset use var while xor"
                , new FontStyle(&phpKeywords, NULL, TFontStyles() << fsBold), at);

  AddWord(L"$", new FontStyle(&phpVariable, NULL, TFontStyles() << fsBold), at);

  phpQuoteTree = AddPushPopPair(L"\"", L"\"", new FontStyle(&phpQuotes, &phpQuotesBack), at, NULL, MASK_PHP_QUOTE);
  AddPushPopPair(L"'", L"'", new FontStyle(&phpQuotes, &phpQuotesBack), at, phpQuoteTree, MASK_PHP_APOSTROPHE);
  AddKeywords(L"\\\"", new FontStyle(), phpQuoteTree);
  //phpQuoteTree->Name = L"php quotes";

  FontStyle * commentStyle = new FontStyle(&phpComments, bg);
  phpCommentTree = AddPair(L"/*", L"*/", commentStyle, at);
  AddLine(L"//", commentStyle, at, phpCommentTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::AddSafetyJumps()
{
  TreeNode* array[10] = {htmlTree, htmlCommentTree, htmlTagTree,
  cssPropertyTree, cssValueTree, cssEnterTree,
  cssTree,  jvTree, jvEnterTree,
  jvCommentTree};

  AddJumpFront(L"<", NULL, LangDefSpecType::Jump, htmlTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION);
  AddJumpFront(L"<", NULL, LangDefSpecType::Jump, cssTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION);
  AddJumpFront(L"<", NULL, LangDefSpecType::Jump, cssPropertyTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION);
  AddJumpFront(L"<", NULL, LangDefSpecType::Jump, cssValueTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION);

  AddJumpFront(L"{", NULL, LangDefSpecType::Jump, htmlTagTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION);
  AddJumpFront(L"{", NULL, LangDefSpecType::Jump, htmlTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION);



  TreeNode* qarray[3] =  {htmlQuoteTree, cssQuoteTree, jvQuoteTree};
  AddJumpFront(L"<", NULL, LangDefSpecType::Jump, htmlQuoteTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION, MASK_APOSTROPHE | MASK_QUOTE | MASK_DOUBLE_QUOTE);
  AddJumpFront(L"<", NULL, LangDefSpecType::Jump, cssQuoteTree, htmlTagTree, MASK_PREDICTION, MASK_PREDICTION, MASK_APOSTROPHE | MASK_QUOTE | MASK_DOUBLE_QUOTE);
  AddJumpFront(L"{", NULL, LangDefSpecType::Jump, htmlQuoteTree, cssPropertyTree, MASK_PREDICTION, MASK_PREDICTION, MASK_APOSTROPHE | MASK_QUOTE | MASK_DOUBLE_QUOTE);
  AddJumpFront(L"}", NULL, LangDefSpecType::Jump, htmlQuoteTree, cssTree, MASK_PREDICTION, MASK_PREDICTION, MASK_APOSTROPHE | MASK_QUOTE | MASK_DOUBLE_QUOTE);


}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructJavascript(LanguageDefinition::TreeNode * at, TColor * bg, TColor * fg)
{
  AddKeywords(L" break const condition ? ifTrue : ifFalse continue delete do...while export for for...in function if...else import in instanceOf label let new return switch this throw try...catch typeof var void while with yield"
                , new FontStyle(&jvKeywords, NULL, TFontStyles() << fsBold), at);

  jvQuoteTree = AddPushPopPair(L"\"", L"\"", new FontStyle(&jvQuotes, bg), at, NULL, MASK_QUOTE);
  AddPushPopPair(L"'", L"'", new FontStyle(&jvQuotes, bg), at, jvQuoteTree, MASK_APOSTROPHE);
  AddPushPopPair(L"\\\"", L"\\\"", new FontStyle(&jvQuotes, bg), at, jvQuoteTree, MASK_DOUBLE_QUOTE);

  AddKeywords(L"\\\"", new FontStyle(), jvQuoteTree);

  FontStyle * commentStyle = new FontStyle(&jvComments, bg);
  jvCommentTree = AddPair(L"/*", L"*/", commentStyle, at);
  AddLine(L"//", commentStyle, at, jvCommentTree);

  AddPops(at);
  AddPops(jvQuoteTree);
  AddPops(jvCommentTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::AddPops(LanguageDefinition::TreeNode * at)
{
  AddPop(L"'", NULL, at, MASK_APOSTROPHE, POP_AUTO);
  AddPop(L"\"", NULL, at, MASK_QUOTE, POP_AUTO);
  AddPop(L"\\\"", NULL, at, MASK_DOUBLE_QUOTE, POP_AUTO);
  AddPop(L"'", NULL, at, MASK_PHP_APOSTROPHE, POP_AUTO);
  AddPop(L"\"", NULL, at, MASK_PHP_QUOTE, POP_AUTO);
}
