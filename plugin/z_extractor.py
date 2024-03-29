# /*****************************************************************************
# *                                                                            *
# *  Copyright (C) 2022-2023 Liu An Lin <liuanlin-mx@qq.com>                   *
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
import signal
import stat
import wx
import subprocess
import json
import time
import platform
import threading


###########################################################################
## Class z_extractor_base
###########################################################################

class z_extractor_base ( wx.Dialog ):

	def __init__( self, parent ):
		wx.Dialog.__init__ ( self, parent, id = wx.ID_ANY, title = u"z_extractor", pos = wx.DefaultPosition, size = wx.Size( 900,640 ), style = wx.DEFAULT_DIALOG_STYLE|wx.MAXIMIZE_BOX|wx.RESIZE_BORDER )

		self.SetSizeHints( wx.Size( 800,640 ), wx.DefaultSize )

		bSizer4 = wx.BoxSizer( wx.VERTICAL )

		bSizer1 = wx.BoxSizer( wx.HORIZONTAL )

		sbSizer1 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Net Classes" ), wx.VERTICAL )

		m_listBoxNetClassesChoices = []
		self.m_listBoxNetClasses = wx.ListBox( sbSizer1.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxNetClassesChoices, wx.LB_EXTENDED|wx.LB_HSCROLL|wx.LB_SORT )
		sbSizer1.Add( self.m_listBoxNetClasses, 1, wx.EXPAND, 5 )

		bSizer11 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonRefresh = wx.Button( sbSizer1.GetStaticBox(), wx.ID_ANY, u"refresh", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer11.Add( self.m_buttonRefresh, 0, wx.EXPAND, 5 )

		self.m_textCtrlNetClassesFilter = wx.TextCtrl( sbSizer1.GetStaticBox(), wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer11.Add( self.m_textCtrlNetClassesFilter, 0, wx.EXPAND, 5 )


		sbSizer1.Add( bSizer11, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer1, 1, wx.EXPAND, 5 )

		sbSizer7 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"T-Line" ), wx.VERTICAL )

		m_listBoxTLineChoices = []
		self.m_listBoxTLine = wx.ListBox( sbSizer7.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxTLineChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer7.Add( self.m_listBoxTLine, 1, wx.EXPAND, 5 )

		bSizer9 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonTLineAdd = wx.Button( sbSizer7.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer9.Add( self.m_buttonTLineAdd, 0, wx.EXPAND, 5 )

		self.m_buttonTLineDel = wx.Button( sbSizer7.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer9.Add( self.m_buttonTLineDel, 0, wx.EXPAND, 5 )


		sbSizer7.Add( bSizer9, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer7, 1, wx.EXPAND, 5 )

		sbSizer3 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Coupled" ), wx.VERTICAL )

		m_listBoxCoupledChoices = []
		self.m_listBoxCoupled = wx.ListBox( sbSizer3.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCoupledChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer3.Add( self.m_listBoxCoupled, 1, wx.EXPAND, 5 )

		sbSizer71 = wx.StaticBoxSizer( wx.StaticBox( sbSizer3.GetStaticBox(), wx.ID_ANY, u"threshold" ), wx.VERTICAL )

		bSizer6 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText2 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"MinLen:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText2.Wrap( -1 )

		bSizer6.Add( self.m_staticText2, 1, wx.ALL, 5 )

		self.m_textCtrlMinLen = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, u"1", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_textCtrlMinLen.SetMaxLength( 10 )
		bSizer6.Add( self.m_textCtrlMinLen, 0, wx.EXPAND, 5 )


		sbSizer71.Add( bSizer6, 1, wx.EXPAND, 5 )

		bSizer7 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText1 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"MaxGap:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText1.Wrap( -1 )

		bSizer7.Add( self.m_staticText1, 1, wx.ALL, 5 )

		self.m_textCtrlMaxDist = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, u"0.508", wx.DefaultPosition, wx.Size( -1,-1 ), 0 )
		self.m_textCtrlMaxDist.SetMaxLength( 10 )
		bSizer7.Add( self.m_textCtrlMaxDist, 0, wx.EXPAND, 5 )


		sbSizer71.Add( bSizer7, 1, wx.EXPAND, 5 )


		sbSizer3.Add( sbSizer71, 0, wx.EXPAND, 5 )

		bSizer61 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_buttonCoupledAdd = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonCoupledAdd, 1, wx.EXPAND, 5 )

		self.m_buttonCoupledDel = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonCoupledDel, 1, wx.EXPAND, 5 )


		sbSizer3.Add( bSizer61, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer3, 1, wx.EXPAND, 5 )

		sbSizer2 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Ref Net" ), wx.VERTICAL )

		m_listBoxRefNetChoices = []
		self.m_listBoxRefNet = wx.ListBox( sbSizer2.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxRefNetChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer2.Add( self.m_listBoxRefNet, 1, wx.EXPAND, 5 )

		bSizer10 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonRefNetAdd = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer10.Add( self.m_buttonRefNetAdd, 0, wx.EXPAND, 5 )

		self.m_buttonRefNetDel = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer10.Add( self.m_buttonRefNetDel, 0, wx.EXPAND, 5 )


		sbSizer2.Add( bSizer10, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer2, 1, wx.EXPAND, 5 )

		sbSizer4 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"cfg" ), wx.VERTICAL )

		m_listBoxCfgChoices = []
		self.m_listBoxCfg = wx.ListBox( sbSizer4.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxCfgChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer4.Add( self.m_listBoxCfg, 1, wx.EXPAND, 5 )

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

		sbSizer6 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"options" ), wx.HORIZONTAL )

		m_radioBoxSpiceFmtChoices = [ u"ngspice (txl)", u"ngspice (ltra)" ]
		self.m_radioBoxSpiceFmt = wx.RadioBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Spice Format", wx.DefaultPosition, wx.DefaultSize, m_radioBoxSpiceFmtChoices, 1, wx.RA_SPECIFY_COLS )
		self.m_radioBoxSpiceFmt.SetSelection( 0 )
		sbSizer6.Add( self.m_radioBoxSpiceFmt, 1, wx.ALL, 5 )

		m_radioBoxSolverChoices = [ u"mmtl", u"fdm" ]
		self.m_radioBoxSolver = wx.RadioBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Solver", wx.DefaultPosition, wx.DefaultSize, m_radioBoxSolverChoices, 1, wx.RA_SPECIFY_COLS )
		self.m_radioBoxSolver.SetSelection( 0 )
		sbSizer6.Add( self.m_radioBoxSolver, 1, wx.ALL, 5 )

		m_radioBoxUnitChoices = [ u"mm", u"mil" ]
		self.m_radioBoxUnit = wx.RadioBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Unit", wx.DefaultPosition, wx.DefaultSize, m_radioBoxUnitChoices, 1, wx.RA_SPECIFY_COLS )
		self.m_radioBoxUnit.SetSelection( 1 )
		sbSizer6.Add( self.m_radioBoxUnit, 1, wx.ALL|wx.EXPAND, 5 )

		m_radioBoxViaModelChoices = [ u"LC", u"TL" ]
		self.m_radioBoxViaModel = wx.RadioBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Via Model", wx.DefaultPosition, wx.DefaultSize, m_radioBoxViaModelChoices, 1, wx.RA_SPECIFY_COLS )
		self.m_radioBoxViaModel.SetSelection( 1 )
		sbSizer6.Add( self.m_radioBoxViaModel, 0, wx.ALL|wx.EXPAND, 5 )

		self.m_checkBoxLosslessTL = wx.CheckBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Lossless TL", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_checkBoxLosslessTL.SetValue(True)
		sbSizer6.Add( self.m_checkBoxLosslessTL, 1, wx.ALL|wx.EXPAND, 5 )

		bSizer71 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticTextStep = wx.StaticText( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Scan Step:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticTextStep.Wrap( -1 )

		bSizer71.Add( self.m_staticTextStep, 0, wx.ALIGN_CENTER|wx.ALL, 5 )

		self.m_textCtrlStep = wx.TextCtrl( sbSizer6.GetStaticBox(), wx.ID_ANY, u"0.5", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_textCtrlStep.SetMaxLength( 10 )
		bSizer71.Add( self.m_textCtrlStep, 0, wx.ALIGN_CENTER|wx.ALL, 5 )


		sbSizer6.Add( bSizer71, 1, wx.EXPAND, 5 )

		bSizer8 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonExtract = wx.Button( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Extract", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer8.Add( self.m_buttonExtract, 0, wx.ALIGN_CENTER|wx.ALL, 5 )

		self.m_checkBoxExtractAll = wx.CheckBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Extract All", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer8.Add( self.m_checkBoxExtractAll, 0, wx.ALIGN_CENTER|wx.ALL, 5 )


		sbSizer6.Add( bSizer8, 1, wx.ALL|wx.EXPAND, 5 )


		bSizer4.Add( sbSizer6, 0, wx.EXPAND, 5 )

		self.m_textCtrlOutput = wx.TextCtrl( self, wx.ID_ANY, wx.EmptyString, wx.DefaultPosition, wx.DefaultSize, wx.TE_MULTILINE|wx.TE_READONLY )
		bSizer4.Add( self.m_textCtrlOutput, 1, wx.ALL|wx.EXPAND, 5 )


		self.SetSizer( bSizer4 )
		self.Layout()
		self.m_timer = wx.Timer()
		self.m_timer.SetOwner( self, wx.ID_ANY )

		self.Centre( wx.BOTH )

		# Connect Events
		self.m_buttonRefresh.Bind( wx.EVT_BUTTON, self.m_buttonRefreshOnButtonClick )
		self.m_textCtrlNetClassesFilter.Bind( wx.EVT_TEXT, self.m_textCtrlNetClassesFilterOnText )
		self.m_buttonTLineAdd.Bind( wx.EVT_BUTTON, self.m_buttonTLineAddOnButtonClick )
		self.m_buttonTLineDel.Bind( wx.EVT_BUTTON, self.m_buttonTLineDelOnButtonClick )
		self.m_textCtrlMinLen.Bind( wx.EVT_TEXT, self.m_textCtrlMinLenOnText )
		self.m_textCtrlMaxDist.Bind( wx.EVT_TEXT, self.m_textCtrlMaxDistOnText )
		self.m_buttonCoupledAdd.Bind( wx.EVT_BUTTON, self.m_buttonCoupledAddOnButtonClick )
		self.m_buttonCoupledDel.Bind( wx.EVT_BUTTON, self.m_buttonCoupledDelOnButtonClick )
		self.m_buttonRefNetAdd.Bind( wx.EVT_BUTTON, self.m_buttonRefNetAddOnButtonClick )
		self.m_buttonRefNetDel.Bind( wx.EVT_BUTTON, self.m_buttonRefNetDelOnButtonClick )
		self.m_listBoxCfg.Bind( wx.EVT_LISTBOX, self.m_listBoxCfgOnListBox )
		self.m_buttonCfgAdd.Bind( wx.EVT_BUTTON, self.m_buttonCfgAddOnButtonClick )
		self.m_buttonCfgDel.Bind( wx.EVT_BUTTON, self.m_buttonCfgDelOnButtonClick )
		self.m_buttonCfgRename.Bind( wx.EVT_BUTTON, self.m_buttonCfgRenameOnButtonClick )
		self.m_buttonSave.Bind( wx.EVT_BUTTON, self.m_buttonSaveOnButtonClick )
		self.m_radioBoxSpiceFmt.Bind( wx.EVT_RADIOBOX, self.m_radioBoxSpiceFmtOnRadioBox )
		self.m_radioBoxSolver.Bind( wx.EVT_RADIOBOX, self.m_radioBoxSolverOnRadioBox )
		self.m_radioBoxUnit.Bind( wx.EVT_RADIOBOX, self.m_radioBoxUnitOnRadioBox )
		self.m_radioBoxViaModel.Bind( wx.EVT_RADIOBOX, self.m_radioBoxViaModelOnRadioBox )
		self.m_checkBoxLosslessTL.Bind( wx.EVT_CHECKBOX, self.m_checkBoxLosslessTLOnCheckBox )
		self.m_textCtrlStep.Bind( wx.EVT_TEXT, self.m_textCtrlStepOnText )
		self.m_buttonExtract.Bind( wx.EVT_BUTTON, self.m_buttonExtractOnButtonClick )
		self.Bind( wx.EVT_TIMER, self.m_timerOnTimer, id=wx.ID_ANY )

	def __del__( self ):
		pass


	# Virtual event handlers, override them in your derived class
	def m_buttonRefreshOnButtonClick( self, event ):
		event.Skip()

	def m_textCtrlNetClassesFilterOnText( self, event ):
		event.Skip()

	def m_buttonTLineAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonTLineDelOnButtonClick( self, event ):
		event.Skip()

	def m_textCtrlMinLenOnText( self, event ):
		event.Skip()

	def m_textCtrlMaxDistOnText( self, event ):
		event.Skip()

	def m_buttonCoupledAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonCoupledDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonRefNetAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonRefNetDelOnButtonClick( self, event ):
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

	def m_radioBoxSpiceFmtOnRadioBox( self, event ):
		event.Skip()

	def m_radioBoxSolverOnRadioBox( self, event ):
		event.Skip()

	def m_radioBoxUnitOnRadioBox( self, event ):
		event.Skip()

	def m_radioBoxViaModelOnRadioBox( self, event ):
		event.Skip()

	def m_checkBoxLosslessTLOnCheckBox( self, event ):
		event.Skip()

	def m_textCtrlStepOnText( self, event ):
		event.Skip()

	def m_buttonExtractOnButtonClick( self, event ):
		event.Skip()

	def m_timerOnTimer( self, event ):
		event.Skip()



class z_config_item():
    def __init__(self):
        self.tline = []
        self.ref_net = ["GND"]
        self.coupled = []
        self.name = "newcfg"
        self.spice_fmt = 0
        self.coupled_max_gap = 0.508
        self.coupled_min_len = 0.0254 * 100
        self.solver_type = 0
        self.lossless_tl = True
        self.scan_step = 0.508
        self.via_model = 0
        

class z_extractor_gui(z_extractor_base):
    def __init__(self):
        z_extractor_base.__init__(self, None)
        self.unit = "mil"
        self.cfg_list = []
        self.cur_cfg = z_config_item()
        self.net_classes_list = []
        self.board = pcbnew.GetBoard()
        self.sub_process_pid = -1
        self.thread = None
        self.lock = threading.Lock()
        self.cmd_line = ""
        self.cmd_output = ""
        
        file_name = self.board.GetFileName()
        self.board_path = os.path.split(file_name)[0]
        self.plugin_path = os.path.split(os.path.realpath(__file__))[0]
        
        self.output_path = self.board_path + os.sep + "z_extractor"
        self.cfg_path = self.board_path + os.sep + "z_extractor" + os.sep + "z_extractor.json"
        if not os.path.exists(self.output_path):
            os.mkdir(self.output_path)
        
        sys = platform.system()
        if sys == "Windows":
            self.plugin_bin_path = self.plugin_path + os.sep + "win"
        elif sys == "Linux":
            self.plugin_bin_path = self.plugin_path + os.sep + "linux"
            os.chmod(self.plugin_path + os.sep + "linux" + os.sep + "mmtl_bem", stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP)
            os.chmod(self.plugin_path + os.sep + "linux" + os.sep + "z_extractor", stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP)
            os.chmod(self.plugin_path + os.sep + "linux" + os.sep + "fasthenry", stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP)
        else:
            pass
        
        self.load_cfg()
        
        self.update_net_classes_ui()
        self.update_cfg_ui()
        self.update_tline_ui()
        self.update_coupled_ui()
        self.update_ref_net_ui()
        self.update_spice_fmt_ui();
        self.update_other_ui()
    
    def load_cfg(self):
        old_cfg = False
        try:
            cfg_file = open(self.cfg_path, "r")
        except:
            try:
                cfg_file = open(self.board_path + os.sep  + "z_extractor.json", "r")
                old_cfg = True
            except:
                self.cfg_list.append(z_config_item())
                self.cur_cfg = self.cfg_list[0]
                return
            
        if old_cfg:
            cfg_list = json.load(cfg_file)
        else:
            json_data = json.load(cfg_file)
            self.unit = json_data["unit"]
            cfg_list = json_data["cfg_list"]
            
        cfg_file.close()
        
        for cfg in cfg_list:
            item = z_config_item()
            if "name" in cfg:
                item.name = cfg["name"]
            if "tline" in cfg:
                item.tline = cfg["tline"]
            if "coupled" in cfg:
                item.coupled = cfg["coupled"]
            if "ref_net" in cfg :
                item.ref_net = cfg["ref_net"]
            if "coupled_max_gap" in cfg:
                item.coupled_max_gap = cfg["coupled_max_gap"]
            if "coupled_min_len" in cfg:
                item.coupled_min_len = cfg["coupled_min_len"]
            if "spice_fmt" in cfg:
                item.spice_fmt = cfg["spice_fmt"]
            if "solver_type" in cfg:
                item.solver_type = cfg["solver_type"]
            if "lossless_tl" in cfg:
                item.lossless_tl = cfg["lossless_tl"]
            if "scan_step" in cfg:
                item.scan_step = cfg["scan_step"]
            if "via_model" in cfg:
                item.via_model = cfg["via_model"]
            
            self.cfg_list.append(item)
        
        if len(self.cfg_list) == 0:
            self.cfg_list.append(z_config_item())
        
        self.cur_cfg = self.cfg_list[0]
        
    def gen_cmd(self, all = True):
        cmd = ""
        if all:
            cfg_list = self.cfg_list
        else:
            cfg_list = [self.cur_cfg]
            
        for cfg in cfg_list:
            cmd = cmd + "z_extractor -pcb " + self.board.GetFileName() + " -t "
            
            if len(cfg.ref_net) > 0:
                cmd = cmd + '-ref "'
                for ref in cfg.ref_net:
                    cmd = cmd + ref + ","
                cmd = cmd.strip(',') + '" '
                
            if len(cfg.tline) > 0:
                cmd = cmd + '-net "'
                for tline in cfg.tline:
                    cmd = cmd + tline + ","
                cmd = cmd.strip(',') + '" '
                
            if len(cfg.coupled) > 0:
                cmd = cmd + '-coupled "'
                for coupled in cfg.coupled:
                    cmd = cmd + coupled + ","
                cmd = cmd.strip(',') + '" '
                
            cmd = cmd + "-coupled_max_gap " + str(cfg.coupled_max_gap) + " -coupled_min_len " + str(cfg.coupled_min_len) + " "
            if cfg.spice_fmt == 0:
                cmd = cmd + "-ltra 0 "
            elif cfg.spice_fmt == 1:
                cmd = cmd + "-ltra 1 "
                
            if cfg.solver_type == 0:
                cmd = cmd + "-mmtl 1 "
            elif cfg.solver_type == 1:
                cmd = cmd + "-mmtl 0 "
                
            if cfg.lossless_tl:
                cmd = cmd + "-lossless_tl 1 "
            else:
                cmd = cmd + "-lossless_tl 0 "
                
            if cfg.via_model == 1:
                cmd = cmd + "-via_tl_mode 1 "
            else:
                cmd = cmd + "-via_tl_mode 0 "
            
            cmd = cmd + "-step " + str(cfg.scan_step)
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
            
    
    def update_cfg_ui(self):
        self.m_listBoxCfg.Clear()
        for cfg in self.cfg_list:
            self.m_listBoxCfg.Append(cfg.name)
        
        self.m_listBoxCfg.SetStringSelection(self.cur_cfg.name)
    
    def update_tline_ui(self):
        self.m_listBoxTLine.Clear()
        for tline in self.cur_cfg.tline:
            self.m_listBoxTLine.Append(tline)
            
    def update_ref_net_ui(self):
        self.m_listBoxRefNet.Clear()
        for net in self.cur_cfg.ref_net:
            self.m_listBoxRefNet.Append(net)
            
    def update_coupled_ui(self):
        self.m_listBoxCoupled.Clear()
        for net in self.cur_cfg.coupled:
            self.m_listBoxCoupled.Append(net)
        self.m_textCtrlMaxDist.SetValue(str(self.unit_mm_cvt_ui(self.cur_cfg.coupled_max_gap)))
        self.m_textCtrlMinLen.SetValue(str(self.unit_mm_cvt_ui(self.cur_cfg.coupled_min_len)))
        
        
    def update_spice_fmt_ui(self):
        #self.m_radioBoxSpiceFmt
        if self.cur_cfg.spice_fmt == 0:
            self.m_radioBoxSpiceFmt.Select(0)
        if self.cur_cfg.spice_fmt == 1:
            self.m_radioBoxSpiceFmt.Select(1)
        
    def update_other_ui(self):
        if self.cur_cfg.solver_type == 0:
            self.m_radioBoxSolver.Select(0)
        else:
            self.m_radioBoxSolver.Select(1)
            
        if self.cur_cfg.via_model == 0:
            self.m_radioBoxViaModel.Select(0)
        else:
            self.m_radioBoxViaModel.Select(1)
            
        self.m_checkBoxLosslessTL.SetValue(self.cur_cfg.lossless_tl)
        self.m_textCtrlStep.SetValue(str(self.unit_mm_cvt_ui(self.cur_cfg.scan_step)))
        
        if self.unit == "mm":
            self.m_radioBoxUnit.Select(0)
        else:
            self.m_radioBoxUnit.Select(1)
        
        
    def is_in_coupled(self, net):
        for str in self.cur_cfg.coupled:
            net1 = str.split(":")[0]
            net2 = str.split(":")[1]
            if net == net1 or net == net2:
                return True
        return False
        
    def unit_mm_cvt_ui(self, v):
        if self.unit == "mm":
            return round(v, 4)
        else:
            return round(v / 0.0254, 4)
            
    def unit_ui_cvt_mm(self, v):
        if self.unit == "mm":
            return round(v, 4)
        else:
            return round(v * 0.0254, 4)
            
    # Virtual event handlers, override them in your derived class
    def m_buttonRefreshOnButtonClick( self, event ):
        self.update_net_classes_ui()

    def m_textCtrlNetClassesFilterOnText( self, event ):
        self.update_net_classes_ui()
        
    def m_buttonTLineAddOnButtonClick( self, event ):
        selected = self.m_listBoxNetClasses.GetSelections()
        for n in selected:
            net = self.m_listBoxNetClasses.GetString(n)
            if (net not in self.cur_cfg.tline) and (net not in self.cur_cfg.ref_net):
                self.cur_cfg.tline.append(net)
                self.m_listBoxTLine.Append(net)
        
            
    def m_buttonTLineDelOnButtonClick( self, event ):
        selected = self.m_listBoxTLine.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            net = self.m_listBoxTLine.GetString(selected[size])
            self.m_listBoxTLine.Delete(selected[size])
            self.cur_cfg.tline.remove(net)
            

    def m_textCtrlMinLenOnText( self, event ):
        self.cur_cfg.coupled_min_len = self.unit_ui_cvt_mm(float(self.m_textCtrlMinLen.GetValue()))
        event.Skip()

    def m_textCtrlMaxDistOnText( self, event ):
        self.cur_cfg.coupled_max_gap = self.unit_ui_cvt_mm(float(self.m_textCtrlMaxDist.GetValue()))
        event.Skip()

        
    def m_buttonCoupledAddOnButtonClick( self, event ):
        selected = self.m_listBoxNetClasses.GetSelections()
        if len(selected) != 2:
            return
            
        net1 = self.m_listBoxNetClasses.GetString(selected[0])
        net2 = self.m_listBoxNetClasses.GetString(selected[1])
        if (net1 in self.cur_cfg.ref_net) or (net2 in self.cur_cfg.ref_net):
            return
        
        net = net1 + ":" + net2
        if net not in self.cur_cfg.coupled:
            self.cur_cfg.coupled.append(net)
            self.m_listBoxCoupled.Append(net)

    def m_buttonCoupledDelOnButtonClick( self, event ):
        selected = self.m_listBoxCoupled.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            net = self.m_listBoxCoupled.GetString(selected[size])
            self.cur_cfg.coupled.remove(net)
            self.m_listBoxCoupled.Delete(selected[size])

    def m_buttonRefNetAddOnButtonClick( self, event ):
        selected = self.m_listBoxNetClasses.GetSelections()
        for n in selected:
            net = self.m_listBoxNetClasses.GetString(n)
            if (net not in self.cur_cfg.tline) and (net not in self.cur_cfg.ref_net) and (not self.is_in_coupled(net)):
                self.cur_cfg.ref_net.append(net)
                self.m_listBoxRefNet.Append(net)

    def m_buttonRefNetDelOnButtonClick( self, event ):
        selected = self.m_listBoxRefNet.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            net = self.m_listBoxRefNet.GetString(selected[size])
            self.m_listBoxRefNet.Delete(selected[size])
            self.cur_cfg.ref_net.remove(net)

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
        
        self.update_tline_ui()
        self.update_coupled_ui()
        self.update_ref_net_ui()
        self.update_spice_fmt_ui();
        self.update_other_ui()
        
    def m_buttonCfgAddOnButtonClick( self, event ):
        dlg = wx.TextEntryDialog(None, u"cfg name", u"name:")
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "" and self.m_listBoxCfg.FindString(name) < 0:
                cfg = z_config_item()
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
        self.update_tline_ui()
        self.update_coupled_ui()
        self.update_ref_net_ui()
        
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

    def m_radioBoxSpiceFmtOnRadioBox( self, event ):
        self.cur_cfg.spice_fmt = self.m_radioBoxSpiceFmt.GetSelection()
        
    def m_radioBoxSolverOnRadioBox( self, event ):
        self.cur_cfg.solver_type = self.m_radioBoxSolver.GetSelection()
            
    def m_radioBoxUnitOnRadioBox( self, event ):
        self.unit = self.m_radioBoxUnit.GetStringSelection()
        self.update_coupled_ui()
        self.update_other_ui()
        
    def m_radioBoxViaModelOnRadioBox( self, event ):
        self.cur_cfg.via_model = self.m_radioBoxViaModel.GetSelection()
        
    def m_checkBoxLosslessTLOnCheckBox( self, event ):
        self.cur_cfg.lossless_tl = self.m_checkBoxLosslessTL.GetValue()

    def m_textCtrlStepOnText( self, event ):
        self.cur_cfg.scan_step = self.unit_ui_cvt_mm(float(self.m_textCtrlStep.GetValue()))

    def m_buttonExtractOnButtonClick( self, event ):
        if self.m_buttonExtract.GetLabel() == "Terminate":
            sys = platform.system()
            if sys == "Windows":
                #os.kill(self.sub_process_pid, signal.CTRL_C_EVENT)
                os.popen("taskkill.exe /f /t /pid " + str(self.sub_process_pid))
            elif sys == "Linux":
                os.killpg(os.getpgid(self.sub_process_pid), signal.SIG_IGN)
            else:
                pass
            self.m_buttonExtract.SetLabel("Extract")
            self.m_timer.Stop()
            self.thread.join()
            return
        
        self.lock.acquire()
        self.cmd_output = ""
        self.cmd_line = self.gen_cmd(self.m_checkBoxExtractAll.GetValue())
        self.lock.release()
        
        self.m_textCtrlOutput.AppendText("cmd: " + self.cmd_line + "\n\n")
        self.m_buttonExtract.SetLabel("Terminate")
        self.m_timer.Start(1000)
        
        self.thread = threading.Thread(target=z_extractor_gui.thread_func, args=(self,))
        self.thread.start()
        
        
    def m_buttonSaveOnButtonClick( self, event ):
        list = []
        for cfg in self.cfg_list:
            list.append(cfg.__dict__)
            
        js = {"unit": self.unit, "cfg_list": list}
        
        json_str = json.dumps(js)
        
        cfg_file = open(self.cfg_path, "w")
        json.dump(js, cfg_file)
        cfg_file.close()
        
    def m_timerOnTimer( self, event ):
        cmd_output = ""
        if self.lock.acquire(blocking=False):
            cmd_output = self.cmd_output
            self.lock.release()
            
        if (cmd_output == ""):
            self.m_textCtrlOutput.AppendText(".")
        else:
            self.m_textCtrlOutput.AppendText("\n" + cmd_output + "\n");
            self.m_buttonExtract.SetLabel("Extract")
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
            
            
            
        
class z_extractor( pcbnew.ActionPlugin ):
    def defaults( self ):
        self.name = "Z Extractor"
        self.category = "Z Extractor"
        self.description = "Transmission Line Impedance Extraction"
        self.show_toolbar_button = True
        self.icon_file_name = os.path.join(os.path.dirname(__file__), 'icon.png')

    def Run( self ):
        dig = z_extractor_gui()
        dig.Show()
        
z_extractor().register()
