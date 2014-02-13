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
  phpBG = (TColor)0xFFF5F5;    //blue
  phpVariable = (TColor)0x0000CC;
  phpQuotes = clRed;
  phpQuotesBack = phpBG;
  phpComments = clGreen;

  cssBG = (TColor)0xF5FFF5;        //green
  cssFG = htmlFG;
  cssAttr = (TColor)0x00AA00;
  cssVal = clBlue;

  SetDefaultColor(&htmlFG);
  SetCaseSensitive(false);

  //html
  TreeItem * htmlTree = GetTree();
  TreeItem * commentTree = NULL;
  TreeItem * tagTree = NULL;

  ConstructHtml(htmlTree, commentTree, tagTree, &htmlBG, &htmlFG);

  TreeItem * phpTree = AddPair(L"<?", L"?>", new FontStyle(&phpFG,&phpBG), htmlTree, NULL);
  AddPair(L"<?", L"?>", new FontStyle(&phpFG,&phpBG), tagTree, phpTree);
  AddPair(L"<?", L"?>", new FontStyle(&phpFG,&phpBG), commentTree, phpTree);

  //php
  TreeItem* phpCommentTree = NULL;
  ConstructPhp(phpTree, phpCommentTree, &phpBG, &phpFG);
  AddJump(L"?>", new FontStyle(&phpComments, &phpBG), LangDefSpecType::Jump, phpCommentTree, htmlTree);

  //css
  TreeItem * cssPropertyTree = NULL;
  TreeItem * cssValueTree = NULL;
  TreeItem * cssEnterTree = AddDupTree(tagTree, new FontStyle(&cssFG, &cssBG));
  TreeItem * cssTree = AddNewTree( new FontStyle(&cssFG, &cssBG));
  AddJump(L"style", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, tagTree, cssEnterTree);
  AddJump(L"<style", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, htmlTree, cssEnterTree);
  AddPush(L"style='", new FontStyle(&htmlTags, NULL), tagTree, cssPropertyTree, 0, MASK_APOSTROPHE);
  AddPush(L"style=\"", new FontStyle(&htmlTags, NULL), tagTree, cssPropertyTree, 0, MASK_QUOTE);
  AddPush(L"style=\\\"", new FontStyle(&htmlTags, NULL), tagTree, cssPropertyTree, 0, MASK_DOUBLE_QUOTE);
  cssEnterTree->map[(int)'>'] = NULL;
  AddJump(L">", tagTree->format, LangDefSpecType::Jump, cssEnterTree, cssTree);
  AddJump(L"</style", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, cssTree, tagTree);
  AddJump(L"</style>", new FontStyle(&htmlTags, &cssBG), LangDefSpecType::Jump, cssTree, htmlTree);
  ConstructCss(cssTree, cssPropertyTree, cssValueTree, &cssBG, &cssFG);
  AddPops(cssEnterTree);

}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructHtml(LanguageDefinition::TreeItem * at, LanguageDefinition::TreeItem *& commentTree,LanguageDefinition::TreeItem *& tagTree, TColor * bg, TColor * fg)
{
  tagTree = AddPair(L"<", L">", new FontStyle(&htmlTags, bg), at);
  tagTree->format = new FontStyle(fg, bg);
  AddReservedNames(L"a abbr acronym address applet area article aside audio b base basefont bdi bdo bgsound big blink blockquote body br button canvas caption center cite code col colgroup content data datalist dd decorator del details dfn dir div dl dt element em embed fieldset figcaption figure font footer form frame frameset h1 h2 h3 h4 h5 h6 head header hgroup hr html i iframe img input ins isindex kbd keygen label legend li link listing main map mark marquee menu menuitem meta meter nav nobr noframes noscript object ol optgroup option output p param plaintext pre progress q rp rt ruby s samp script section select shadow small source spacer span strike strong style sub summary sup table tbody td template textarea tfoot th thead time title tr track tt u ul var video wbr xmp"
                    , new FontStyle(&htmlTags, NULL), tagTree);
  AddReservedNames(L"accept accept-charset accesskey action align alt async autocomplete autofocus autoplay bgcolor buffered challenge charset checked cite class code codebase color cols colspan content contenteditable contextmenu controls coords data datetime default defer dir dirname disabled download draggable dropzone enctype for form headers height hidden high href hreflang http-equiv icon id ismap itemprop keytype kind label lang language list loop low manifest max maxlength media method min multiple name novalidate open optimum pattern ping placeholder poster preload pubdate radiogroup readonly rel required reversed rows rowspan sandbox spellcheck scope scoped seamless selected shape size sizes span src srcdoc srclang start step style summary tabindex target title type usemap value width wrap "
                    , new FontStyle(&htmlAttribs, NULL), tagTree);
  TreeItem * quoteTree = AddPair(L"\"", L"\"", new FontStyle(&htmlQuotes, bg), tagTree);
  AddPair(L"\\\"", L"\\\"", new FontStyle(&htmlQuotes, bg), tagTree, quoteTree);
  AddPair(L"'", L"'", new FontStyle(&htmlQuotes, bg), tagTree, quoteTree);

  commentTree = AddPair(L"<!--", L"-->", new FontStyle(&htmlComments, bg));

  AddPops(commentTree);
  AddPops(at);
  AddPops(tagTree);
  AddPops(quoteTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructCss(LanguageDefinition::TreeItem * at, LanguageDefinition::TreeItem *& propertyTree, LanguageDefinition::TreeItem *& valueTree, TColor * bg, TColor * fg)
{
  FontStyle * tagStyle = new FontStyle(&htmlTags, bg);
  AddReservedNames(L"a abbr acronym address applet area article aside audio b base basefont bdi bdo bgsound big blink blockquote body br button canvas caption center cite code col colgroup content data datalist dd decorator del details dfn dir div dl dt element em embed fieldset figcaption figure font footer form frame frameset h1 h2 h3 h4 h5 h6 head header hgroup hr html i iframe img input ins isindex kbd keygen label legend li link listing main map mark marquee menu menuitem meta meter nav nobr noframes noscript object ol optgroup option output p param plaintext pre progress q rp rt ruby s samp script section select shadow small source spacer span strike strong style sub summary sup table tbody td template textarea tfoot th thead time title tr track tt u ul var video wbr xmp"
                  , tagStyle, at);
  AddWord(L".", tagStyle, at);
  AddWord(L"#", tagStyle, at);
  AddWord(L":", tagStyle, at);

  propertyTree = AddPair(L"{", L"}", at->format, at);
  AddReservedNames( L"azimuth background-attachment background-color background-image background-position background-repeat background border-collapse border-color border-spacing border-style border-top border-right border-bottom border-left border-top-color border-right-color border-bottom-color border-left-color border-top-style border-right-style border-bottom-style border-left-style border-top-width border-right-width border-bottom-width border-left-width border-width border bottom caption-side clear clip color content counter-increment counter-reset cue-after cue-before cue cursor direction display elevationempty-cells float font-family font-size font-style font-variant font-weight font height left letter-spacing line-height list-style-image list-style-position list-style-type list-style margin-right margin-left margin-top margin-bottom margin max-height max-width min-height min-width orphans outline-color outline-style outline-width outline overflow padding-top padding-right padding-bottom padding-left padding page-break-after page-break-before page-break-inside pause-after pause-before pause pitch-range pitch play-during position quotes richness right speak-header speak-numeral speak-punctuation speak speech-rate stress table-layout text-align text-decoration text-indent text-transform top unicode-bidi vertical-align visibility voice-family volume white-space widows width word-spacing z-index"
                , new FontStyle(&cssAttr, bg), propertyTree);

  FontStyle * defstyle = new FontStyle(fg, bg);
  valueTree = AddNewTree(defstyle);
  AddJump(L"}", defstyle, LangDefSpecType::Jump, valueTree, at);
  AddJump(L":", defstyle, LangDefSpecType::Jump, propertyTree, valueTree);
  AddJump(L";", defstyle, LangDefSpecType::Jump, valueTree, propertyTree);
  AddReservedNames(L"above absolute always armenian auto avoid baseline behind below bidi-override blink block bold bolder both bottom capitalize caption center center-left center-right circle close-quote code collapse continuous crosshair 'cue-after' cue-before' decimal decimal-leading-zero default digits disc embed e-resize far-left far-right fast faster fixed georgian help hidden hide high higher icon inherit inline inline-block inline-table inside italic justify left left-side leftwards level lighter line-through list-item loud low lower lower-alpha lowercase lower-greek lower-latin lower-roman ltr medium menu message-box middle mix move ne-resize no-close-quote none no-open-quote no-repeat normal nowrap n-resize nw-resize oblique once open-quote outside overline pointer pre pre-line pre-wrap progress relative repeat repeat-x repeat-y right right-side rightwards rtl scroll separate se-resize show silent slow slower small small-caps small-caption soft spell-out square s-resize static status-bar sub super sw-resize table table-caption table-cell table-column table-column-group table-footer-group table-header-group table-row table-row-group text text-bottom text-top top transparentnone underline upper-alpha uppercase upper-latin upper-roman visible visible wait w-resize x-fast x-high x-loud x-low x-slow x-soft"
                , new FontStyle(&cssVal, bg), valueTree);

  AddPops(at);
  AddPops(propertyTree);
  AddPops(valueTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::ConstructPhp(LanguageDefinition::TreeItem * at, LanguageDefinition::TreeItem *& commentTree,  TColor * bg, TColor * fg)
{
  AddReservedNames(L"__halt_compiler abstract and array as break callable case catch class clone const continue declare default die do echo else elseif empty enddeclare endfor endforeach endif endswitch endwhile eval exit extends final for foreach function global goto if implements include include_once instanceof insteadof interface isset list namespace new or print private protected public require require_once return static switch throw trait try unset use var while xor"
                , new FontStyle(&phpKeywords, NULL, TFontStyles() << fsBold), at);

  AddWord(L"$", new FontStyle(&phpVariable, NULL, TFontStyles() << fsBold), at);

  TreeItem * quoteTree = AddPair(L"\"", L"\"", new FontStyle(&phpQuotes, &phpQuotesBack), at);
  AddPair(L"'", L"'", new FontStyle(&phpQuotes, &phpQuotesBack), at, quoteTree);
  AddReservedNames(L"\\\"", new FontStyle(), quoteTree);

  FontStyle * commentStyle = new FontStyle(&phpComments, bg);
  commentTree = AddPair(L"/*", L"*/", commentStyle, at);
  AddJump(L"//", commentStyle, LangDefSpecType::LineTag, at, commentTree);
}
//---------------------------------------------------------------------------
void LanguageDefinitionWebLangs::AddPops(LanguageDefinition::TreeItem * at)
{
  AddPop(L"'", NULL, at, MASK_APOSTROPHE);
  AddPop(L"\"", NULL, at, MASK_QUOTE);
  AddPop(L"\\\"", NULL, at, MASK_DOUBLE_QUOTE);
}
