#!/usr/bin/env python

import pcbnew
import os
import wx

###########################################################################
## Class z_extractor_base
###########################################################################

class z_extractor_base ( wx.Dialog ):

	def __init__( self, parent ):
		wx.Dialog.__init__ ( self, parent, id = wx.ID_ANY, title = wx.EmptyString, pos = wx.DefaultPosition, size = wx.Size( 815,622 ), style = wx.DEFAULT_DIALOG_STYLE )

		self.SetSizeHints( wx.DefaultSize, wx.DefaultSize )

		bSizer4 = wx.BoxSizer( wx.VERTICAL )

		bSizer1 = wx.BoxSizer( wx.HORIZONTAL )

		sbSizer1 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Net Classes" ), wx.VERTICAL )

		m_listBoxNetClassesChoices = []
		self.m_listBoxNetClasses = wx.ListBox( sbSizer1.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxNetClassesChoices, wx.LB_MULTIPLE|wx.LB_SORT )
		sbSizer1.Add( self.m_listBoxNetClasses, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonRefresh = wx.Button( sbSizer1.GetStaticBox(), wx.ID_ANY, u"refresh", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer1.Add( self.m_buttonRefresh, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer1, 1, wx.EXPAND, 5 )

		sbSizer7 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"T-Line" ), wx.VERTICAL )

		m_listBoxTLineChoices = []
		self.m_listBoxTLine = wx.ListBox( sbSizer7.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxTLineChoices, wx.LB_MULTIPLE )
		sbSizer7.Add( self.m_listBoxTLine, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonTLineAdd = wx.Button( sbSizer7.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer7.Add( self.m_buttonTLineAdd, 0, wx.ALL, 5 )

		self.m_buttonTLineDel = wx.Button( sbSizer7.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer7.Add( self.m_buttonTLineDel, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer7, 1, wx.EXPAND, 5 )

		sbSizer3 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Coupled" ), wx.VERTICAL )

		m_listBoxCoupledChoices = []
		self.m_listBoxCoupled = wx.ListBox( sbSizer3.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCoupledChoices, wx.LB_MULTIPLE )
		sbSizer3.Add( self.m_listBoxCoupled, 1, wx.ALL|wx.EXPAND, 5 )

		sbSizer71 = wx.StaticBoxSizer( wx.StaticBox( sbSizer3.GetStaticBox(), wx.ID_ANY, u"threshold" ), wx.VERTICAL )

		bSizer6 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText2 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"Min Len:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText2.Wrap( -1 )

		bSizer6.Add( self.m_staticText2, 0, wx.ALL, 5 )

		self.m_textCtrlMinLen = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, u"0.5", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer6.Add( self.m_textCtrlMinLen, 0, 0, 5 )


		sbSizer71.Add( bSizer6, 1, wx.EXPAND, 5 )

		bSizer7 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText1 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"Max Dist:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText1.Wrap( -1 )

		bSizer7.Add( self.m_staticText1, 0, wx.ALL, 5 )

		self.m_textCtrlMaxDist = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, u"2", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer7.Add( self.m_textCtrlMaxDist, 0, 0, 5 )


		sbSizer71.Add( bSizer7, 1, wx.EXPAND, 5 )


		sbSizer3.Add( sbSizer71, 0, wx.EXPAND, 5 )

		bSizer61 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_buttonCoupledAdd = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonCoupledAdd, 0, wx.ALL, 5 )

		self.m_buttonCoupledDel = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonCoupledDel, 0, wx.ALL, 5 )


		sbSizer3.Add( bSizer61, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer3, 1, wx.EXPAND, 5 )

		sbSizer2 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Ref Net" ), wx.VERTICAL )

		m_listBoxRefNetChoices = []
		self.m_listBoxRefNet = wx.ListBox( sbSizer2.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxRefNetChoices, wx.LB_MULTIPLE )
		sbSizer2.Add( self.m_listBoxRefNet, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonRefNetAdd = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer2.Add( self.m_buttonRefNetAdd, 0, wx.ALL, 5 )

		self.m_buttonRefNetDel = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer2.Add( self.m_buttonRefNetDel, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer2, 1, wx.EXPAND, 5 )

		sbSizer4 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"cfg" ), wx.VERTICAL )

		m_listBoxCfgChoices = []
		self.m_listBoxCfg = wx.ListBox( sbSizer4.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCfgChoices, wx.LB_MULTIPLE )
		sbSizer4.Add( self.m_listBoxCfg, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonCfgAdd = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer4.Add( self.m_buttonCfgAdd, 0, wx.ALL, 5 )

		self.m_buttonCfgDel = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer4.Add( self.m_buttonCfgDel, 0, wx.ALL, 5 )

		self.m_buttonCfgRename = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"rename", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer4.Add( self.m_buttonCfgRename, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer4, 1, wx.EXPAND, 5 )


		bSizer4.Add( bSizer1, 1, wx.EXPAND, 5 )

		sbSizer6 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"options" ), wx.HORIZONTAL )

		m_radioBoxSpiceFmtChoices = [ u"ngspice(txl)", u"ngspice (ltra)" ]
		self.m_radioBoxSpiceFmt = wx.RadioBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Spice Format", wx.DefaultPosition, wx.DefaultSize, m_radioBoxSpiceFmtChoices, 1, wx.RA_SPECIFY_COLS )
		self.m_radioBoxSpiceFmt.SetSelection( 0 )
		sbSizer6.Add( self.m_radioBoxSpiceFmt, 0, wx.ALL, 5 )

		bSizer8 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonExtract = wx.Button( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Extract", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer8.Add( self.m_buttonExtract, 0, wx.ALL, 5 )

		self.m_buttonSave = wx.Button( sbSizer6.GetStaticBox(), wx.ID_ANY, u"save", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer8.Add( self.m_buttonSave, 0, wx.ALL, 5 )


		sbSizer6.Add( bSizer8, 1, wx.EXPAND, 5 )


		bSizer4.Add( sbSizer6, 0, wx.EXPAND, 5 )

		self.m_textCtrlOutput = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, wx.TE_MULTILINE|wx.TE_READONLY )
		bSizer4.Add( self.m_textCtrlOutput, 1, wx.ALL|wx.EXPAND, 5 )


		self.SetSizer( bSizer4 )
		self.Layout()

		self.Centre( wx.BOTH )

		# Connect Events
		self.m_buttonRefresh.Bind( wx.EVT_BUTTON, self.m_buttonRefreshOnButtonClick )
		self.m_buttonTLineAdd.Bind( wx.EVT_BUTTON, self.m_buttonTLineAddOnButtonClick )
		self.m_buttonTLineDel.Bind( wx.EVT_BUTTON, self.m_buttonTLineDelOnButtonClick )
		self.m_buttonCoupledAdd.Bind( wx.EVT_BUTTON, self.m_buttonCoupledAddOnButtonClick )
		self.m_buttonCoupledDel.Bind( wx.EVT_BUTTON, self.m_buttonCoupledDelOnButtonClick )
		self.m_buttonRefNetAdd.Bind( wx.EVT_BUTTON, self.m_buttonRefNetAddOnButtonClick )
		self.m_buttonRefNetDel.Bind( wx.EVT_BUTTON, self.m_buttonRefNetDelOnButtonClick )
		self.m_buttonCfgAdd.Bind( wx.EVT_BUTTON, self.m_buttonCfgAddOnButtonClick )
		self.m_buttonCfgDel.Bind( wx.EVT_BUTTON, self.m_buttonCfgDelOnButtonClick )
		self.m_buttonCfgRename.Bind( wx.EVT_BUTTON, self.m_buttonCfgRenameOnButtonClick )
		self.m_radioBoxSpiceFmt.Bind( wx.EVT_RADIOBOX, self.m_radioBoxSpiceFmtOnRadioBox )
		self.m_buttonExtract.Bind( wx.EVT_BUTTON, self.m_buttonExtractOnButtonClick )
		self.m_buttonSave.Bind( wx.EVT_BUTTON, self.m_buttonSaveOnButtonClick )

	def __del__( self ):
		pass


	# Virtual event handlers, override them in your derived class
	def m_buttonRefreshOnButtonClick( self, event ):
		event.Skip()

	def m_buttonTLineAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonTLineDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCoupledAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCoupledDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonRefNetAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonRefNetDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCfgAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCfgDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCfgRenameOnButtonClick( self, event ):
		event.Skip()

	def m_radioBoxSpiceFmtOnRadioBox( self, event ):
		event.Skip()

	def m_buttonExtractOnButtonClick( self, event ):
		event.Skip()

	def m_buttonSaveOnButtonClick( self, event ):
		event.Skip()




class z_config_item():
    def __init__(self):
        self.tline = [];
        

class z_extractor_gui(z_extractor_base):
    def __init__(self):
        z_extractor_base.__init__(self, None)
        self.cfg_dick = {}
        self.cur_cfg = z_config_item()
        self.cur_cfg_name = "test"
        self.net_classes_list = []
        self.board = pcbnew.GetBoard()
        
            
        file_name = self.board.GetFileName()
        self.path = os.path.split(file_name)[0]
        file_name = os.path.split(file_name)[1]
        name = file_name.rsplit('.',1)[0]
        
        self.update_net_classes_ui()
        
            
    def update_net_classes_ui(self):
        self.m_listBoxNetClasses.Clear()
        self.net_classes_list = []
        netcodes = self.board.GetNetsByNetcode()
        for netcode, net in netcodes.items():
            if net.GetNetname() not in self.net_classes_list:
                self.net_classes_list.append(net.GetNetname())
                self.m_listBoxNetClasses.Append(net.GetNetname())
        
        tracks = self.board.GetTracks()
        for track in tracks:
            if track.IsSelected():
                net = track.GetNet()
                self.m_textCtrlOutput.AppendText("{}\n".format(track))
                if net.GetNetname() in self.net_classes_list:
                    self.m_listBoxNetClasses.SetStringSelection(net.GetNetname())
            
    
    def update_tline_ui(self):
        self.m_listBoxTLine.Clear()
        for tline in self.cur_cfg.tline:
            self.m_listBoxTLine.Append(tline)
            
        
    # Virtual event handlers, override them in your derived class
    def m_buttonRefreshOnButtonClick( self, event ):
        self.update_net_classes_ui()

    def m_buttonTLineAddOnButtonClick( self, event ):
        selected = self.m_listBoxNetClasses.GetSelections()
        for n in selected:
            str = self.m_listBoxNetClasses.GetString(n)
            if str not in self.cur_cfg.tline:
                self.cur_cfg.tline.append(str)
                self.m_listBoxTLine.Append(str)
        
            
    def m_buttonTLineDelOnButtonClick( self, event ):
        selected = self.m_listBoxTLine.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            str = self.m_listBoxTLine.GetString(selected[size])
            self.m_listBoxTLine.Delete(selected[size])
            self.cur_cfg.tline.remove(str)
            

    def m_buttonCoupledAddOnButtonClick( self, event ):
        event.Skip()

    def m_buttonCoupledDelOnButtonClick( self, event ):
        event.Skip()

    def m_buttonRefNetAddOnButtonClick( self, event ):
        event.Skip()

    def m_buttonRefNetDelOnButtonClick( self, event ):
        event.Skip()

    def m_buttonCfgAddOnButtonClick( self, event ):
        event.Skip()

    def m_buttonCfgDelOnButtonClick( self, event ):
        event.Skip()

    def m_buttonCfgRenameOnButtonClick( self, event ):
        event.Skip()

    def m_radioBoxSpiceFmtOnRadioBox( self, event ):
        event.Skip()

    def m_buttonExtractOnButtonClick( self, event ):
        event.Skip()

    def m_buttonSaveOnButtonClick( self, event ):
        event.Skip()
        
class z_extractor( pcbnew.ActionPlugin ):
    def defaults( self ):
        self.name = "Impedance Extractor"
        self.category = "Impedance Extractor"
        self.description = "Impedance Extractor"
        self.show_toolbar_button = True

    def Run( self ):
        dig = z_extractor_gui()
        dig.Show()
        
z_extractor().register()
