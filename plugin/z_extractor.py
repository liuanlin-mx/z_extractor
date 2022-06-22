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
		self.m_listBoxNetClasses = wx.ListBox( sbSizer1.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxNetClassesChoices, wx.LB_HSCROLL|wx.LB_MULTIPLE|wx.LB_SORT )
		sbSizer1.Add( self.m_listBoxNetClasses, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonRefresh = wx.Button( sbSizer1.GetStaticBox(), wx.ID_ANY, u"refresh", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer1.Add( self.m_buttonRefresh, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer1, 1, wx.EXPAND, 5 )

		sbSizer7 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"T-Line" ), wx.VERTICAL )

		m_listBoxTLineChoices = []
		self.m_listBoxTLine = wx.ListBox( sbSizer7.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxTLineChoices, 0 )
		sbSizer7.Add( self.m_listBoxTLine, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonTLineAdd = wx.Button( sbSizer7.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer7.Add( self.m_buttonTLineAdd, 0, wx.ALL, 5 )

		self.m_buttonTLineDel = wx.Button( sbSizer7.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer7.Add( self.m_buttonTLineDel, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer7, 1, wx.EXPAND, 5 )

		sbSizer3 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Coupled" ), wx.VERTICAL )

		m_listBoxCoupledChoices = []
		self.m_listBoxCoupled = wx.ListBox( sbSizer3.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCoupledChoices, 0 )
		sbSizer3.Add( self.m_listBoxCoupled, 1, wx.ALL|wx.EXPAND, 5 )

		sbSizer71 = wx.StaticBoxSizer( wx.StaticBox( sbSizer3.GetStaticBox(), wx.ID_ANY, u"threshold" ), wx.VERTICAL )

		bSizer6 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText2 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"Min Len:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText2.Wrap( -1 )

		bSizer6.Add( self.m_staticText2, 0, wx.ALL, 5 )

		self.m_textCtrl2 = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer6.Add( self.m_textCtrl2, 0, 0, 5 )


		sbSizer71.Add( bSizer6, 1, wx.EXPAND, 5 )

		bSizer7 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText1 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"Max Dist:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText1.Wrap( -1 )

		bSizer7.Add( self.m_staticText1, 0, wx.ALL, 5 )

		self.m_textCtrl1 = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer7.Add( self.m_textCtrl1, 0, 0, 5 )


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
		self.m_listBoxRefNet = wx.ListBox( sbSizer2.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxRefNetChoices, wx.LB_HSCROLL|wx.LB_MULTIPLE )
		sbSizer2.Add( self.m_listBoxRefNet, 1, wx.ALL|wx.EXPAND, 5 )

		self.m_buttonRefNetAdd = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer2.Add( self.m_buttonRefNetAdd, 0, wx.ALL, 5 )

		self.m_buttonRefNetDel = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		sbSizer2.Add( self.m_buttonRefNetDel, 0, wx.ALL, 5 )


		bSizer1.Add( sbSizer2, 1, wx.EXPAND, 5 )

		sbSizer4 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"cfg" ), wx.VERTICAL )

		m_listBoxCfgChoices = []
		self.m_listBoxCfg = wx.ListBox( sbSizer4.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCfgChoices, 0 )
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
		self.m_radioBoxSpiceFmt.SetSelection( 2 )
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

	def __del__( self ):
		pass




class z_extractor_gui(z_extractor_base):
    def __init__(self):
        z_extractor_base.__init__(self, None)
        board = pcbnew.GetBoard()
        netcodes = board.GetNetsByNetcode()
        net_dict = {}
        
        # list off all of the nets in the board.
        for netcode, net in netcodes.items():
            print("netcode {}, name {}".format(netcode, net.GetNetname()))
            net_dict[net.GetNetname()] = self.m_listBoxNetClasses.Append(net.GetNetname())
            
        fName = board.GetFileName()
        path = os.path.split(fName)[0]
        fName = os.path.split(fName)[1]
        bomName = fName.rsplit('.',1)[0]
        
        tracks = board.GetTracks()
        for track in tracks:
            net = track.GetNet()
            net.GetNetname()
            if track.IsSelected():
                if net_dict.get(net.GetNetname()) != None:
                    self.m_listBoxNetClasses.SetSelection(net_dict[net.GetNetname()])
                #self.m_textCtrl1.AppendText(net.GetNetname() + "\n")
            print(track)
            
            
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
