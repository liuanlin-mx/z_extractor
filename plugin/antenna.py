# /*****************************************************************************
# *                                                                            *
# *  Copyright (C) 2023 Liu An Lin <liuanlin-mx@qq.com>                        *
# *                                                                            *
# *  Licensed under the Apache License, Version 2.0 (the "License");           *
# *  you may not use this file except in compliance with the License.          *
# *  You may obtain a copy of the License at                                   *
# *                                                                            *
# *      http://www.apache.org/licenses/LICENSE-2.0                            *
# *                                                                            *
# *  Unless required by applicable law or agreed to in writing, software       *
# *  distributed under the License is distributed on an "AS IS" BASIS,         *
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
# *  See the License for the specific language governing permissions and       *
# *  limitations under the License.                                            *
# *                                                                            *
# *****************************************************************************/

import pcbnew
import os
import stat
import wx
import wx.grid
import subprocess
import json
import time
import platform
import signal
import threading

###########################################################################
## Class antenna_base
###########################################################################

class antenna_base ( wx.Dialog ):

	def __init__( self, parent ):
		wx.Dialog.__init__ ( self, parent, id = wx.ID_ANY, title = u"ANT", pos = wx.DefaultPosition, size = wx.Size( 913,640 ), style = wx.DEFAULT_DIALOG_STYLE|wx.MAXIMIZE_BOX|wx.RESIZE_BORDER )

		self.SetSizeHints( wx.Size( 800,640 ), wx.DefaultSize )

		bSizer4 = wx.BoxSizer( wx.VERTICAL )

		bSizer1 = wx.BoxSizer( wx.HORIZONTAL )

		sbSizer1 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Net Classes" ), wx.VERTICAL )

		m_listBoxNetClassesChoices = []
		self.m_listBoxNetClasses = wx.ListBox( sbSizer1.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxNetClassesChoices, wx.LB_EXTENDED|wx.LB_HSCROLL|wx.LB_SORT )
		sbSizer1.Add( self.m_listBoxNetClasses, 1, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		bSizer11 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonRefresh = wx.Button( sbSizer1.GetStaticBox(), wx.ID_ANY, u"refresh", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer11.Add( self.m_buttonRefresh, 0, wx.EXPAND, 5 )

		self.m_textCtrlNetClassesFilter = wx.TextCtrl( sbSizer1.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer11.Add( self.m_textCtrlNetClassesFilter, 0, wx.EXPAND, 5 )


		sbSizer1.Add( bSizer11, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer1, 2, wx.EXPAND, 5 )

		sbSizer2 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Net" ), wx.VERTICAL )

		m_listBoxNetChoices = []
		self.m_listBoxNet = wx.ListBox( sbSizer2.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxNetChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer2.Add( self.m_listBoxNet, 1, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		bSizer10 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonNetAdd = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer10.Add( self.m_buttonNetAdd, 0, wx.EXPAND, 5 )

		self.m_buttonNetDel = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer10.Add( self.m_buttonNetDel, 0, wx.EXPAND, 5 )


		sbSizer2.Add( bSizer10, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer2, 2, wx.EXPAND, 5 )

		sbSizer11 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Fp List" ), wx.VERTICAL )

		m_listBoxFpChoices = []
		self.m_listBoxFp = wx.ListBox( sbSizer11.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxFpChoices, wx.LB_EXTENDED|wx.LB_HSCROLL|wx.LB_SORT )
		sbSizer11.Add( self.m_listBoxFp, 1, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		bSizer111 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonRefreshFp = wx.Button( sbSizer11.GetStaticBox(), wx.ID_ANY, u"refresh", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer111.Add( self.m_buttonRefreshFp, 0, wx.EXPAND, 5 )

		self.m_textCtrlFpFilter = wx.TextCtrl( sbSizer11.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer111.Add( self.m_textCtrlFpFilter, 0, wx.EXPAND, 5 )


		sbSizer11.Add( bSizer111, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer11, 1, wx.EXPAND, 5 )

		sbSizer3 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Fp" ), wx.VERTICAL )

		m_listBoxFpAddedChoices = []
		self.m_listBoxFpAdded = wx.ListBox( sbSizer3.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxFpAddedChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer3.Add( self.m_listBoxFpAdded, 0, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		bSizer61 = wx.BoxSizer( wx.VERTICAL )

		sbSizer8 = wx.StaticBoxSizer( wx.StaticBox( sbSizer3.GetStaticBox(), wx.ID_ANY, u"NF2FF Box" ), wx.VERTICAL )

		m_choiceNF2FFBoxChoices = []
		self.m_choiceNF2FFBox = wx.Choice( sbSizer8.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_choiceNF2FFBoxChoices, 0 )
		self.m_choiceNF2FFBox.SetSelection( 0 )
		sbSizer8.Add( self.m_choiceNF2FFBox, 0, wx.EXPAND, 5 )


		bSizer61.Add( sbSizer8, 0, wx.EXPAND, 5 )

		self.m_buttonFpAdd = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonFpAdd, 1, wx.EXPAND, 5 )

		self.m_buttonFpDel = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonFpDel, 1, wx.EXPAND, 5 )


		sbSizer3.Add( bSizer61, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer3, 1, wx.EXPAND, 5 )

		sbSizer4 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"cfg" ), wx.VERTICAL )

		m_listBoxCfgChoices = []
		self.m_listBoxCfg = wx.ListBox( sbSizer4.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCfgChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer4.Add( self.m_listBoxCfg, 1, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		gSizer1 = wx.GridSizer( 2, 2, 0, 0 )

		self.m_buttonCfgAdd = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer1.Add( self.m_buttonCfgAdd, 0, wx.EXPAND, 5 )

		self.m_buttonCfgDel = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer1.Add( self.m_buttonCfgDel, 0, wx.EXPAND, 5 )

		self.m_buttonCfgRename = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"rename", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer1.Add( self.m_buttonCfgRename, 0, wx.EXPAND, 5 )

		self.m_buttonSave = wx.Button( sbSizer4.GetStaticBox(), wx.ID_ANY, u"save", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer1.Add( self.m_buttonSave, 0, wx.EXPAND, 5 )


		sbSizer4.Add( gSizer1, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer4, 1, wx.EXPAND, 5 )


		bSizer4.Add( bSizer1, 1, wx.EXPAND, 5 )

		bSizer81 = wx.BoxSizer( wx.HORIZONTAL )

		sbSizer21 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Excitation" ), wx.VERTICAL )

		self.m_gridEx = wx.grid.Grid( sbSizer21.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, 0 )

		# Grid
		self.m_gridEx.CreateGrid( 0, 6 )
		self.m_gridEx.EnableEditing( True )
		self.m_gridEx.EnableGridLines( True )
		self.m_gridEx.EnableDragGridSize( False )
		self.m_gridEx.SetMargins( 0, 0 )

		# Columns
		self.m_gridEx.EnableDragColMove( False )
		self.m_gridEx.EnableDragColSize( True )
		self.m_gridEx.SetColLabelValue( 0, u"Pad1" )
		self.m_gridEx.SetColLabelValue( 1, u"Layer1" )
		self.m_gridEx.SetColLabelValue( 2, u"Pad2" )
		self.m_gridEx.SetColLabelValue( 3, u"Layer2" )
		self.m_gridEx.SetColLabelValue( 4, u"R" )
		self.m_gridEx.SetColLabelValue( 5, u"Dir" )
		self.m_gridEx.SetColLabelSize( wx.grid.GRID_AUTOSIZE )
		self.m_gridEx.SetColLabelAlignment( wx.ALIGN_CENTER, wx.ALIGN_CENTER )

		# Rows
		self.m_gridEx.EnableDragRowSize( True )
		self.m_gridEx.SetRowLabelSize( wx.grid.GRID_AUTOSIZE )
		self.m_gridEx.SetRowLabelAlignment( wx.ALIGN_CENTER, wx.ALIGN_CENTER )

		# Label Appearance

		# Cell Defaults
		self.m_gridEx.SetDefaultCellAlignment( wx.ALIGN_LEFT, wx.ALIGN_TOP )
		sbSizer21.Add( self.m_gridEx, 1, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		bSizer101 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_buttonExAddLine = wx.Button( sbSizer21.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer101.Add( self.m_buttonExAddLine, 1, wx.EXPAND, 5 )

		self.m_buttonExDelLine = wx.Button( sbSizer21.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer101.Add( self.m_buttonExDelLine, 1, 0, 5 )


		sbSizer21.Add( bSizer101, 0, wx.EXPAND, 5 )


		bSizer81.Add( sbSizer21, 2, wx.EXPAND, 5 )

		sbSizerMesh = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"MeshLines" ), wx.VERTICAL )

		self.m_gridMesh = wx.grid.Grid( sbSizerMesh.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, 0 )

		# Grid
		self.m_gridMesh.CreateGrid( 0, 4 )
		self.m_gridMesh.EnableEditing( True )
		self.m_gridMesh.EnableGridLines( True )
		self.m_gridMesh.EnableDragGridSize( False )
		self.m_gridMesh.SetMargins( 0, 0 )

		# Columns
		self.m_gridMesh.EnableDragColMove( False )
		self.m_gridMesh.EnableDragColSize( True )
		self.m_gridMesh.SetColLabelValue( 0, u"Start(mm)" )
		self.m_gridMesh.SetColLabelValue( 1, u"End(mm)" )
		self.m_gridMesh.SetColLabelValue( 2, u"Gap(mm)" )
		self.m_gridMesh.SetColLabelValue( 3, u"Dir" )
		self.m_gridMesh.SetColLabelValue( 4, wx.EmptyString )
		self.m_gridMesh.SetColLabelSize( wx.grid.GRID_AUTOSIZE )
		self.m_gridMesh.SetColLabelAlignment( wx.ALIGN_CENTER, wx.ALIGN_CENTER )

		# Rows
		self.m_gridMesh.EnableDragRowSize( True )
		self.m_gridMesh.SetRowLabelSize( wx.grid.GRID_AUTOSIZE )
		self.m_gridMesh.SetRowLabelAlignment( wx.ALIGN_CENTER, wx.ALIGN_CENTER )

		# Label Appearance

		# Cell Defaults
		self.m_gridMesh.SetDefaultCellAlignment( wx.ALIGN_LEFT, wx.ALIGN_TOP )
		sbSizerMesh.Add( self.m_gridMesh, 1, wx.EXPAND|wx.FIXED_MINSIZE, 5 )

		bSizer9 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_buttonMeshAddLine = wx.Button( sbSizerMesh.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer9.Add( self.m_buttonMeshAddLine, 1, wx.EXPAND, 5 )

		self.m_buttonMeshDelLine = wx.Button( sbSizerMesh.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer9.Add( self.m_buttonMeshDelLine, 1, wx.EXPAND, 5 )


		sbSizerMesh.Add( bSizer9, 0, wx.EXPAND, 5 )


		bSizer81.Add( sbSizerMesh, 2, wx.EXPAND, 5 )

		sbSizer6 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"options" ), wx.HORIZONTAL )

		gSizer2 = wx.GridSizer( 0, 2, 0, 0 )

		self.m_staticText5 = wx.StaticText( sbSizer6.GetStaticBox(), wx.ID_ANY, u"End Criteria(db):", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText5.Wrap( -1 )

		gSizer2.Add( self.m_staticText5, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT|wx.ALL, 5 )

		self.m_textCtrlEndCriteria = wx.TextCtrl( sbSizer6.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_textCtrlEndCriteria, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, 5 )

		self.m_staticText2 = wx.StaticText( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Max Freq:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText2.Wrap( -1 )

		gSizer2.Add( self.m_staticText2, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT|wx.ALL, 5 )

		self.m_textCtrlMaxFreq = wx.TextCtrl( sbSizer6.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_textCtrlMaxFreq, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, 5 )

		self.m_staticText4 = wx.StaticText( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Freq:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText4.Wrap( -1 )

		gSizer2.Add( self.m_staticText4, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT|wx.ALL, 5 )

		self.m_textCtrlFreq = wx.TextCtrl( sbSizer6.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_textCtrlFreq, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, 5 )

		self.m_buttonGenerate = wx.Button( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Generate", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_buttonGenerate, 0, wx.ALIGN_CENTER, 5 )

		self.m_checkBoxExtractAll = wx.CheckBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Generate All", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_checkBoxExtractAll, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, 5 )

		self.m_buttonRun = wx.Button( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Run", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_buttonRun, 0, wx.ALIGN_CENTER, 5 )

		self.m_checkBoxOnlyPlot = wx.CheckBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Plot Only", wx.DefaultPosition, wx.DefaultSize, 0 )
		gSizer2.Add( self.m_checkBoxOnlyPlot, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, 5 )


		sbSizer6.Add( gSizer2, 1, wx.EXPAND, 5 )


		bSizer81.Add( sbSizer6, 1, wx.ALIGN_CENTER|wx.EXPAND, 5 )


		bSizer4.Add( bSizer81, 1, wx.EXPAND, 5 )

		self.m_textCtrlOutput = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, wx.TE_MULTILINE|wx.TE_READONLY )
		bSizer4.Add( self.m_textCtrlOutput, 1, wx.ALL|wx.EXPAND, 5 )


		self.SetSizer( bSizer4 )
		self.Layout()
		self.m_timer = wx.Timer()
		self.m_timer.SetOwner( self, wx.ID_ANY )

		self.Centre( wx.BOTH )

		# Connect Events
		self.m_listBoxNetClasses.Bind( wx.EVT_LISTBOX, self.m_listBoxNetClassesOnListBox )
		self.m_buttonRefresh.Bind( wx.EVT_BUTTON, self.m_buttonRefreshOnButtonClick )
		self.m_textCtrlNetClassesFilter.Bind( wx.EVT_TEXT, self.m_textCtrlNetClassesFilterOnText )
		self.m_buttonNetAdd.Bind( wx.EVT_BUTTON, self.m_buttonNetAddOnButtonClick )
		self.m_buttonNetDel.Bind( wx.EVT_BUTTON, self.m_buttonNetDelOnButtonClick )
		self.m_listBoxFp.Bind( wx.EVT_LISTBOX, self.m_listBoxNetClassesOnListBox )
		self.m_buttonRefreshFp.Bind( wx.EVT_BUTTON, self.m_buttonRefreshFpOnButtonClick )
		self.m_textCtrlFpFilter.Bind( wx.EVT_TEXT, self.m_textCtrlFpFilterOnText )
		self.m_choiceNF2FFBox.Bind( wx.EVT_CHOICE, self.m_choiceNF2FFBoxOnChoice )
		self.m_buttonFpAdd.Bind( wx.EVT_BUTTON, self.m_buttonFpAddOnButtonClick )
		self.m_buttonFpDel.Bind( wx.EVT_BUTTON, self.m_buttonFpDelOnButtonClick )
		self.m_listBoxCfg.Bind( wx.EVT_LISTBOX, self.m_listBoxCfgOnListBox )
		self.m_buttonCfgAdd.Bind( wx.EVT_BUTTON, self.m_buttonCfgAddOnButtonClick )
		self.m_buttonCfgDel.Bind( wx.EVT_BUTTON, self.m_buttonCfgDelOnButtonClick )
		self.m_buttonCfgRename.Bind( wx.EVT_BUTTON, self.m_buttonCfgRenameOnButtonClick )
		self.m_buttonSave.Bind( wx.EVT_BUTTON, self.m_buttonSaveOnButtonClick )
		self.m_gridEx.Bind( wx.grid.EVT_GRID_CELL_CHANGED, self.m_gridExOnGridCellChange )
		self.m_buttonExAddLine.Bind( wx.EVT_BUTTON, self.m_buttonExAddLineOnButtonClick )
		self.m_buttonExDelLine.Bind( wx.EVT_BUTTON, self.m_buttonExDelLineOnButtonClick )
		self.m_gridMesh.Bind( wx.grid.EVT_GRID_CELL_CHANGED, self.m_gridMeshOnGridCellChange )
		self.m_buttonMeshAddLine.Bind( wx.EVT_BUTTON, self.m_buttonMeshAddLineOnButtonClick )
		self.m_buttonMeshDelLine.Bind( wx.EVT_BUTTON, self.m_buttonMeshDelLineOnButtonClick )
		self.m_textCtrlEndCriteria.Bind( wx.EVT_TEXT, self.m_textCtrlEndCriteriaOnText )
		self.m_textCtrlMaxFreq.Bind( wx.EVT_TEXT, self.m_textCtrlMaxFreqOnText )
		self.m_textCtrlFreq.Bind( wx.EVT_TEXT, self.m_textCtrlFreqOnText )
		self.m_buttonGenerate.Bind( wx.EVT_BUTTON, self.m_buttonGenerateOnButtonClick )
		self.m_buttonRun.Bind( wx.EVT_BUTTON, self.m_buttonRunOnButtonClick )
		self.Bind( wx.EVT_TIMER, self.m_timerOnTimer, id=wx.ID_ANY )

	def __del__( self ):
		pass


	# Virtual event handlers, override them in your derived class
	def m_listBoxNetClassesOnListBox( self, event ):
		event.Skip()

	def m_buttonRefreshOnButtonClick( self, event ):
		event.Skip()

	def m_textCtrlNetClassesFilterOnText( self, event ):
		event.Skip()

	def m_buttonNetAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonNetDelOnButtonClick( self, event ):
		event.Skip()


	def m_buttonRefreshFpOnButtonClick( self, event ):
		event.Skip()

	def m_textCtrlFpFilterOnText( self, event ):
		event.Skip()

	def m_choiceNF2FFBoxOnChoice( self, event ):
		event.Skip()

	def m_buttonFpAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonFpDelOnButtonClick( self, event ):
		event.Skip()

	def m_listBoxCfgOnListBox( self, event ):
		event.Skip()

	def m_buttonCfgAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCfgDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCfgRenameOnButtonClick( self, event ):
		event.Skip()

	def m_buttonSaveOnButtonClick( self, event ):
		event.Skip()

	def m_gridExOnGridCellChange( self, event ):
		event.Skip()

	def m_buttonExAddLineOnButtonClick( self, event ):
		event.Skip()

	def m_buttonExDelLineOnButtonClick( self, event ):
		event.Skip()

	def m_gridMeshOnGridCellChange( self, event ):
		event.Skip()

	def m_buttonMeshAddLineOnButtonClick( self, event ):
		event.Skip()

	def m_buttonMeshDelLineOnButtonClick( self, event ):
		event.Skip()

	def m_textCtrlEndCriteriaOnText( self, event ):
		event.Skip()

	def m_textCtrlMaxFreqOnText( self, event ):
		event.Skip()

	def m_textCtrlFreqOnText( self, event ):
		event.Skip()

	def m_buttonGenerateOnButtonClick( self, event ):
		event.Skip()

	def m_buttonRunOnButtonClick( self, event ):
		event.Skip()

	def m_timerOnTimer( self, event ):
		event.Skip()






class ant_config_item():
    def __init__(self):
        self.net = ['GND']
        self.fp = []
        self.nf2ff_box = ""
        self.ex = []
        self.name = "newcfg"
        self.freq = "2.4e9"
        self.max_freq = "5e9"
        self.end_criteria= "-50"
        self.mesh = []
        #for i in range(6):
        #    self.mesh.append({'start': 0, 'end': 0, 'gap': 0.1, 'dir': 'Not used'})
            
        #for i in range(6):
        #    self.ex.append({'pad1': '', 'layer1': '', 'pad2': '', 'layer2': '', 'R': 50, 'dir': 'Not used'})

class antenna_gui(antenna_base):
    def __init__(self):
        antenna_base.__init__(self, None)
        
        self.cfg_list = []
        self.cur_cfg = ant_config_item()
        self.net_classes_list = []
        self.board = pcbnew.GetBoard()
            
        self.sub_process_pid = -1
        self.thread = None
        self.lock = threading.Lock()
        self.cmd_line = ""
        self.cmd_output = ""
        self.std_layer_dict = {self.board.GetStandardLayerName(n): n for n in range(pcbnew.PCB_LAYER_ID_COUNT)}
        self.layer_name = []
        for k, v in self.std_layer_dict.items():
            if self.board.IsLayerEnabled(v) and pcbnew.IsCopperLayer(v):
                    self.layer_name.append(self.board.GetLayerName(v))
                    #self.m_textCtrlOutput.AppendText(self.board.GetLayerName(v) + "\n")
                
                
        file_name = self.board.GetFileName()
        self.board_path = os.path.split(file_name)[0]
        self.plugin_path = os.path.split(os.path.realpath(__file__))[0]
        
        self.output_path = self.board_path + os.sep + "antenna" 
        self.cfg_path = self.board_path + os.sep + "antenna" + os.sep + "antenna.json"
        if not os.path.exists(self.output_path):
            os.mkdir(self.output_path)
        
        sys = platform.system()
        if sys == "Windows":
            self.plugin_bin_path = self.plugin_path + os.sep + "win"
        elif sys == "Linux":
            self.plugin_bin_path = self.plugin_path + os.sep + "linux"
            os.chmod(self.plugin_path + os.sep + "linux" + os.sep + "z_extractor", stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP)
        else:
            pass
            
        self.load_cfg()
        
        
        self.update_net_classes_ui()
        self.update_cfg_ui()
        self.update_fp_list_ui()
        self.update_net_ui()
        self.update_fp_ui()
        self.update_nf2ff_box_ui()
        self.update_excitation_ui()
        self.update_mesh_ui()
        
    def load_cfg(self):
        try:
            cfg_file = open(self.cfg_path, "r")
        except:
            self.cfg_list.append(ant_config_item())
            self.cur_cfg = self.cfg_list[0]
            return
            
        cfg_list = json.load(cfg_file)
        cfg_file.close()
        
        for cfg in cfg_list:
            item = ant_config_item()
            if "name" in cfg:
                item.name = cfg["name"]
            if "net" in cfg:
                item.net = cfg["net"]
            if "fp" in cfg:
                item.fp = cfg["fp"]
            if "ex" in cfg:
                item.ex = cfg["ex"]
            if "freq" in cfg:
                item.freq = cfg["freq"]
            if "max_freq" in cfg:
                item.max_freq = cfg["max_freq"]
            if "end_criteria" in cfg:
                item.end_criteria = cfg["end_criteria"]
            if "nf2ff_box" in cfg:
                item.nf2ff_box = cfg["nf2ff_box"]
            if "mesh" in cfg:
                item.mesh = cfg["mesh"]
                
            self.cfg_list.append(item)
        
        if len(self.cfg_list) == 0:
            self.cfg_list.append(ant_config_item())
        
        self.cur_cfg = self.cfg_list[0]
        
    def gen_cmd(self, all = True):
        cmd = ""
        if all:
            cfg_list = self.cfg_list
        else:
            cfg_list = [self.cur_cfg]
            
        for cfg in cfg_list:
            cmd = cmd + "z_extractor -pcb " + self.board.GetFileName() + " -ant "
            
            if len(cfg.net) > 0:
                cmd = cmd + '-net "'
                for net in cfg.net:
                    cmd = cmd + net + ","
                cmd = cmd.strip(',') + '" '
                
            if len(cfg.fp) > 0:
                for fp in cfg.fp:
                    cmd = cmd + '-fp ' + fp + ' '
                    
            for ex in cfg.ex:
                if len(ex['dir']) > 1:
                    continue
                cmd = cmd + '-port ' + ex['pad1'] + ':' + ex['layer1'] + ':' + ex['pad2'] + ':' + ex['layer2'] + ':' + ex['dir'] + ':' + str(ex['R']) + ':1 '
                    
            if len(cfg.nf2ff_box) > 0:
                    cmd = cmd + '-nf2ff "' + cfg.nf2ff_box + '" '
                    
            for mesh in cfg.mesh:
                if len(mesh['dir']) > 1:
                    continue
                cmd = cmd + '-mesh_range ' + str(mesh['start']) + ':' + str(mesh['end']) + ':' + str(mesh['gap']) + ':' + mesh['dir'] + ' '
                
            cmd = cmd + ' -freq ' + cfg.freq
            cmd = cmd + ' -max_freq ' + cfg.max_freq
            cmd = cmd + ' -criteria ' + cfg.end_criteria
            cmd = cmd + " -o " + cfg.name + ";"
        
        return  cmd.strip(';')
        
    def update_net_classes_ui(self):
        self.m_listBoxNetClasses.Clear()
        self.net_classes_list = []
        netcodes = self.board.GetNetsByNetcode()
        key = self.m_textCtrlNetClassesFilter.GetValue()
        for netcode, net in netcodes.items():
            net_name = net.GetNetname()
            
            if len(key) > 0 and key.lower() not in net_name.lower():
                continue
                
            if netcode != 0 and net_name not in self.net_classes_list:
                self.net_classes_list.append(net_name)
                self.m_listBoxNetClasses.Append(net_name)
        
        tracks = self.board.GetTracks()
        for track in tracks:
            if track.IsSelected():
                net = track.GetNet()
                if net.GetNetname() in self.net_classes_list:
                    self.m_listBoxNetClasses.SetStringSelection(net.GetNetname())
    
    def update_fp_list_ui(self):
        self.m_listBoxFp.Clear()
        key = self.m_textCtrlFpFilter.GetValue()
            
        for m in self.board.GetFootprints():
            footprint = m.GetReference()
            if len(key) > 0 and key.lower() not in footprint.lower():
                continue
            self.m_listBoxFp.Append(footprint)
            
        for m in self.board.GetFootprints():
            if m.IsSelected():
                footprint = m.GetReference()
                #if footprint in self.net_classes_list:
                self.m_listBoxFp.SetStringSelection(footprint)
        
    def update_cfg_ui(self):
        self.m_listBoxCfg.Clear()
        for cfg in self.cfg_list:
            self.m_listBoxCfg.Append(cfg.name)
        
        self.m_listBoxCfg.SetStringSelection(self.cur_cfg.name)
        self.m_textCtrlFreq.SetValue(self.cur_cfg.freq)
        self.m_textCtrlMaxFreq.SetValue(self.cur_cfg.max_freq)
        self.m_textCtrlEndCriteria.SetValue(self.cur_cfg.end_criteria)
    
    def update_net_ui(self):
        self.m_listBoxNet.Clear()
        for net in self.cur_cfg.net:
            self.m_listBoxNet.Append(net)
            
    def update_fp_ui(self):
        self.m_listBoxFpAdded.Clear()
        for fp in self.cur_cfg.fp:
            self.m_listBoxFpAdded.Append(fp)
        
    def update_nf2ff_box_ui(self):
        self.m_choiceNF2FFBox.Clear()
        for fp in self.cur_cfg.fp:
            self.m_choiceNF2FFBox.Append(fp)
        self.m_choiceNF2FFBox.Select(0)
        self.m_choiceNF2FFBox.SetStringSelection(self.cur_cfg.nf2ff_box)
        self.cur_cfg.nf2ff_box = self.m_choiceNF2FFBox.GetStringSelection()
        
    def update_excitation_ui(self):
        choices = []
        for m in self.board.GetFootprints():
            footprint = m.GetReference()
            if footprint in self.cur_cfg.fp:
                for pad in m.Pads():
                    choices.append(footprint + ':' + pad.GetName())
        
        
        self.m_gridEx.DeleteRows(0, self.m_gridEx.GetNumberRows())
        self.m_gridEx.SetDefaultCellAlignment(wx.ALIGN_RIGHT, wx.ALIGN_CENTER)
        
        self.m_gridEx.SetColFormatFloat(4, precision = 0)
        
        for row in range(len(self.cur_cfg.ex)):
            item = self.cur_cfg.ex[row]
            self.m_gridEx.AppendRows()
            self.m_gridEx.SetCellEditor(row, 0, wx.grid.GridCellChoiceEditor(choices))
            self.m_gridEx.SetCellValue(row, 0, str(item['pad1']))
            
            self.m_gridEx.SetCellEditor(row, 1, wx.grid.GridCellChoiceEditor(self.layer_name))
            self.m_gridEx.SetCellValue(row, 1, str(item['layer1']))
            
            self.m_gridEx.SetCellEditor(row, 2, wx.grid.GridCellChoiceEditor(choices))
            self.m_gridEx.SetCellValue(row, 2, str(item['pad2']))
            
            self.m_gridEx.SetCellEditor(row, 3, wx.grid.GridCellChoiceEditor(self.layer_name))
            self.m_gridEx.SetCellValue(row, 3, str(item['layer2']))
            
            self.m_gridEx.SetCellEditor(row, 5, wx.grid.GridCellChoiceEditor(["x", "y", "z"]))
            self.m_gridEx.SetCellValue(row, 5, str(item['dir']))
            
            self.m_gridEx.SetCellValue(row, 4, str(item['R']))
            
        self.m_gridEx.AutoSizeColumns()
        self.m_gridEx.SetRowLabelSize(wx.grid.GRID_AUTOSIZE)
        
    def update_mesh_ui( self ):
        self.m_gridMesh.DeleteRows(0, self.m_gridMesh.GetNumberRows())
        self.m_gridMesh.SetDefaultCellAlignment(wx.ALIGN_RIGHT, wx.ALIGN_CENTER)
        
        self.m_gridMesh.SetColFormatFloat(0, precision = 2)
        self.m_gridMesh.SetColFormatFloat(1, precision = 2)
        self.m_gridMesh.SetColFormatFloat(2, precision = 2)
        
        
        for row in range(len(self.cur_cfg.mesh)):
            self.m_gridMesh.AppendRows()
            self.m_gridMesh.SetCellEditor(row, 3, wx.grid.GridCellChoiceEditor(["x", "y"]))
            item = self.cur_cfg.mesh[row]
            self.m_gridMesh.SetCellValue(row, 0, str(item['start']))
            self.m_gridMesh.SetCellValue(row, 1, str(item['end']))
            self.m_gridMesh.SetCellValue(row, 2, str(item['gap']))
            self.m_gridMesh.SetCellValue(row, 3, str(item['dir']))
            
        self.m_gridMesh.AutoSizeColumns()
        self.m_gridMesh.SetRowLabelSize(wx.grid.GRID_AUTOSIZE)
        
        
    # Virtual event handlers, override them in your derived class
    def m_listBoxNetClassesOnListBox( self, event ):
        event.Skip()
        
    def m_buttonRefreshOnButtonClick( self, event ):
        self.update_net_classes_ui()

    def m_textCtrlNetClassesFilterOnText( self, event ):
        self.update_net_classes_ui()
        
    def m_buttonPadAddOnButtonClick( self, event ):
        selected = self.m_listBoxPads.GetSelections()
        if len(selected) != 2:
            return
            
        pad1 = self.m_listBoxPads.GetString(selected[0])
        pad2 = self.m_listBoxPads.GetString(selected[1])
        
        pad2pad = pad1 + ":" + pad2
        if pad2pad not in self.cur_cfg.pad2pad:
            self.cur_cfg.pad2pad.append(pad2pad)
            self.m_listBoxPadToPad.Append(pad2pad)
        
    def m_buttonPadDelOnButtonClick( self, event ):
        selected = self.m_listBoxPadToPad.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            pad2pad = self.m_listBoxPadToPad.GetString(selected[size])
            self.cur_cfg.pad2pad.remove(pad2pad)
            self.m_listBoxPadToPad.Delete(selected[size])
        

    def m_buttonNetAddOnButtonClick( self, event ):
        selected = self.m_listBoxNetClasses.GetSelections()
        for n in selected:
            net = self.m_listBoxNetClasses.GetString(n)
            if (net not in self.cur_cfg.net):
                self.cur_cfg.net.append(net)
                self.m_listBoxNet.Append(net)
        
    def m_buttonNetDelOnButtonClick( self, event ):
        selected = self.m_listBoxNet.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            net = self.m_listBoxNet.GetString(selected[size])
            self.m_listBoxNet.Delete(selected[size])
            self.cur_cfg.net.remove(net)
    
    

    def m_buttonRefreshFpOnButtonClick( self, event ):
        self.update_fp_list_ui()

    def m_textCtrlFpFilterOnText( self, event ):
        self.update_fp_list_ui()

    def m_choiceNF2FFBoxOnChoice( self, event ):
        self.cur_cfg.nf2ff_box = self.m_choiceNF2FFBox.GetStringSelection()
        
    def m_buttonFpAddOnButtonClick( self, event ):
        selected = self.m_listBoxFp.GetSelections()
        for n in selected:
            fp = self.m_listBoxFp.GetString(n)
            if (fp not in self.cur_cfg.fp):
                self.cur_cfg.fp.append(fp)
                self.m_listBoxFpAdded.Append(fp)
        self.update_nf2ff_box_ui()
        self.update_excitation_ui()

    def m_buttonFpDelOnButtonClick( self, event ):
        selected = self.m_listBoxFpAdded.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            fp = self.m_listBoxFpAdded.GetString(selected[size])
            self.m_listBoxFpAdded.Delete(selected[size])
            self.cur_cfg.fp.remove(fp)
        self.update_nf2ff_box_ui()
        self.update_excitation_ui()
    

    def m_listBoxCfgOnListBox( self, event ):
        selected = self.m_listBoxCfg.GetSelections()
        size = len(selected)
        if size <= 0:
            return
        
        n = selected[0]
        name = self.m_listBoxCfg.GetString(n)
        for cfg in self.cfg_list:
            if cfg.name == name:
                self.cur_cfg = cfg
                break
        
        self.update_cfg_ui()
        self.update_fp_list_ui()
        self.update_net_ui()
        self.update_fp_ui()
        self.update_nf2ff_box_ui()
        self.update_excitation_ui()
        self.update_mesh_ui()
        
    def m_buttonCfgAddOnButtonClick( self, event ):
        dlg = wx.TextEntryDialog(None, u"cfg name", u"name:")
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "" and self.m_listBoxCfg.FindString(name) < 0:
                cfg = ant_config_item()
                cfg.name = name
                self.cfg_list.append(cfg)
                self.m_listBoxCfg.Append(name)
        dlg.Destroy()

    def m_buttonCfgDelOnButtonClick( self, event ):
        selected = self.m_listBoxCfg.GetSelections()
        size = len(selected)
        
        if len(self.cfg_list) == 1 or size <= 0:
            return
            
        while size > 0:
            size = size - 1
            name = self.m_listBoxCfg.GetString(selected[size])
            self.m_listBoxCfg.Delete(selected[size])
            for cfg in self.cfg_list:
                if cfg.name == name:
                    self.cfg_list.remove(cfg)
                    break
            
        self.cur_cfg = self.cfg_list[0]
        self.update_cfg_ui()
        self.update_fp_list_ui()
        self.update_net_ui()
        self.update_fp_ui()
        self.update_nf2ff_box_ui()
        self.update_excitation_ui()
        self.update_mesh_ui()
        
    def m_buttonCfgRenameOnButtonClick( self, event ):
        selected = self.m_listBoxCfg.GetSelections()
        size = len(selected)
        if size <= 0:
            return
        n = selected[0]
        name = self.m_listBoxCfg.GetString(n)
        
        dlg = wx.TextEntryDialog(None, u"cfg name", u"name:", name)
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "" and self.m_listBoxCfg.FindString(name) < 0:
                self.cur_cfg.name = name
                self.m_listBoxCfg.SetString(n, name)
        dlg.Destroy()

    def m_textCtrlEndCriteriaOnText( self, event ):
        self.cur_cfg.end_criteria = self.m_textCtrlEndCriteria.GetValue()

    def m_textCtrlMaxFreqOnText( self, event ):
        self.cur_cfg.max_freq = self.m_textCtrlMaxFreq.GetValue()

    def m_textCtrlFreqOnText( self, event ):
        self.cur_cfg.freq = self.m_textCtrlFreq.GetValue()
        
        
    def m_buttonGenerateOnButtonClick( self, event ):
        if self.m_buttonGenerate.GetLabel() == "Terminate":
            sys = platform.system()
            if sys == "Windows":
                os.popen("taskkill.exe /f /t /pid " + str(self.sub_process_pid))
            elif sys == "Linux":
                os.killpg(os.getpgid(self.sub_process_pid), signal.SIG_IGN)
            else:
                pass
                
            self.m_buttonGenerate.SetLabel("Generate")
            self.m_timer.Stop()
            self.thread.join()
            return
        
        self.lock.acquire()
        self.cmd_output = ""
        self.cmd_line = self.gen_cmd(self.m_checkBoxExtractAll.GetValue())
        self.lock.release()
        
        self.m_textCtrlOutput.AppendText("cmd: " + self.cmd_line + "\n\n")
        self.m_buttonGenerate.SetLabel("Terminate")
        self.m_timer.Start(1000)
        
        self.thread = threading.Thread(target=antenna_gui.thread_func, args=(self,))
        self.thread.start()
        
        
    def m_buttonRunOnButtonClick( self, event ):
        sys = platform.system()
        if sys == "Windows":
            cmd_line = ''
        elif sys == "Linux":
            self.plugin_bin_path = self.plugin_path + os.sep + "linux"
            cmd_line = 'xterm -e "cd ' + self.output_path + ' && octave --silent --persist ' + self.cur_cfg.name + '_ant.m '
            if self.m_checkBoxOnlyPlot.GetValue():
                cmd_line = cmd_line + '--only-plot '
            
            cmd_line = cmd_line + '" &'
        self.m_textCtrlOutput.AppendText(cmd_line + "\n")
        os.system(cmd_line)
        
    def m_buttonSaveOnButtonClick( self, event ):
        json_str = json.dumps(self.cur_cfg.__dict__)
        
        list = []
        for cfg in self.cfg_list:
            list.append(cfg.__dict__)
            
        json_str = json.dumps(list)
        
        cfg_file = open(self.cfg_path, "w")
        json.dump(list, cfg_file)
        cfg_file.close()
        
    def m_gridExOnGridCellChange( self, event ):
        row = event.GetRow()
        item = {}
        item['pad1'] = self.m_gridEx.GetCellValue(row, 0);
        item['layer1'] = self.m_gridEx.GetCellValue(row, 1);
        item['pad2'] = self.m_gridEx.GetCellValue(row, 2);
        item['layer2'] = self.m_gridEx.GetCellValue(row, 3);
        item['R'] = float(self.m_gridEx.GetCellValue(row, 4));
        item['dir'] = self.m_gridEx.GetCellValue(row, 5);
        self.cur_cfg.ex[row] = item
        self.m_gridEx.AutoSizeColumns()

    def m_buttonExAddLineOnButtonClick( self, event ):
        self.cur_cfg.ex.append({'pad1': '', 'layer1': '', 'pad2': '', 'layer2': '', 'R': 50, 'dir': 'x'})
        self.update_excitation_ui()

    def m_buttonExDelLineOnButtonClick( self, event ):
        selected = self.m_gridEx.GetSelectedRows()
        size = len(selected)
        while size > 0:
            size = size - 1
            self.m_gridEx.DeleteRows(selected[size])
            self.cur_cfg.ex.pop(selected[size])
        
    def m_gridMeshOnGridCellChange( self, event ):
        row = event.GetRow()
        item = {}
        item['start'] = float(self.m_gridMesh.GetCellValue(row, 0));
        item['end'] = float(self.m_gridMesh.GetCellValue(row, 1));
        item['gap'] = float(self.m_gridMesh.GetCellValue(row, 2));
        item['dir'] = self.m_gridMesh.GetCellValue(row, 3);
        self.cur_cfg.mesh[row] = item
        self.m_gridMesh.AutoSizeColumns()
        
    def m_buttonMeshAddLineOnButtonClick( self, event ):
        self.cur_cfg.mesh.append({'start': 0, 'end': 0, 'gap': 0.1, 'dir': 'x'})
        self.update_mesh_ui()
        
    def m_buttonMeshDelLineOnButtonClick( self, event ):
        selected = self.m_gridMesh.GetSelectedRows()
        size = len(selected)
        while size > 0:
            size = size - 1
            self.m_gridMesh.DeleteRows(selected[size])
            self.cur_cfg.mesh.pop(selected[size])
        
    def m_timerOnTimer( self, event ):
        cmd_output = ""
        if self.lock.acquire(blocking=False):
            cmd_output = self.cmd_output
            self.lock.release()
            
        if (cmd_output == ""):
            self.m_textCtrlOutput.AppendText(".")
        else:
            self.m_textCtrlOutput.AppendText("\n" + cmd_output + "\n");
            self.m_buttonGenerate.SetLabel("Generate")
            self.m_timer.Stop()
            self.thread.join(0.1)

    def thread_func(self):
        start_time = time.perf_counter()
        self.lock.acquire()
        sys = platform.system()
        if sys == "Windows":
            sub_process = subprocess.Popen(self.cmd_line, shell=True, stdout=subprocess.PIPE, cwd=self.output_path, env={"PATH": self.plugin_bin_path})
        elif sys == "Linux":
            sub_process = subprocess.Popen(self.cmd_line, shell=True, stdout=subprocess.PIPE, cwd=self.output_path, env={"PATH": self.plugin_bin_path}, start_new_session=True)
        else:
            self.cmd_output = "failed"
            self.lock.release()
            return
            
        self.sub_process_pid = sub_process.pid
        self.lock.release()
        out, err = sub_process.communicate()
            
        if sub_process.returncode == 0:
            str = out.decode()
            self.lock.acquire()
            self.cmd_output = str + "\n" + "time: {:.3f}s\n".format((time.perf_counter() - start_time))
            self.lock.release()
            
            
            
        
class antenna( pcbnew.ActionPlugin ):
    def defaults( self ):
        self.name = "Antenna"
        self.category = "Antennas Simulation"
        self.description = "Antennas Simulation"
        self.show_toolbar_button = True
        #self.icon_file_name = os.path.join(os.path.dirname(__file__), 'rl_icon.png')

    def Run( self ):
        dig = antenna_gui()
        dig.Show()
        
antenna().register()
