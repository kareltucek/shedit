object SearchBar: TSearchBar
  Left = 0
  Top = 0
  Caption = 'SearchBar'
  ClientHeight = 60
  ClientWidth = 382
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  DesignSize = (
    382
    60)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 11
    Width = 49
    Height = 13
    Caption = 'Find What'
  end
  object Edit1: TEdit
    Left = 88
    Top = 8
    Width = 286
    Height = 21
    Anchors = [akLeft, akTop, akRight]
    TabOrder = 0
    Text = 'Edit1'
    OnKeyPress = Edit1KeyPress
  end
  object CheckBox1: TCheckBox
    Left = 8
    Top = 35
    Width = 93
    Height = 17
    Alignment = taLeftJustify
    Caption = 'Case Sensitive'
    TabOrder = 1
  end
  object Button1: TButton
    Left = 288
    Top = 31
    Width = 86
    Height = 25
    Caption = 'Find Backward'
    TabOrder = 2
    OnClick = Button1Click
  end
  object Button2: TButton
    Left = 198
    Top = 31
    Width = 85
    Height = 25
    Caption = 'Find Forward'
    TabOrder = 3
    OnClick = Button2Click
  end
end
