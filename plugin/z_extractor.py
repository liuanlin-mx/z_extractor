#!/usr/bin/env python

import pcbnew
import os
import wx
import subprocess
import json

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

		self.m_textCtrlMinLen = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, u"0.1", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_textCtrlMinLen.SetMaxLength( 6 )
		bSizer6.Add( self.m_textCtrlMinLen, 0, 0, 5 )


		sbSizer71.Add( bSizer6, 1, wx.EXPAND, 5 )

		bSizer7 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_staticText1 = wx.StaticText( sbSizer71.GetStaticBox(), wx.ID_ANY, u"Max Dist:", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_staticText1.Wrap( -1 )

		bSizer7.Add( self.m_staticText1, 0, wx.ALL, 5 )

		self.m_textCtrlMaxDist = wx.TextCtrl( sbSizer71.GetStaticBox(), wx.ID_ANY, u"2", wx.DefaultPosition, wx.DefaultSize, 0 )
		self.m_textCtrlMaxDist.SetMaxLength( 6 )
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
		self.m_timer = wx.Timer()
		self.m_timer.SetOwner( self, wx.ID_ANY )

		self.Centre( wx.BOTH )

		# Connect Events
		self.m_buttonRefresh.Bind( wx.EVT_BUTTON, self.m_buttonRefreshOnButtonClick )
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
		self.m_radioBoxSpiceFmt.Bind( wx.EVT_RADIOBOX, self.m_radioBoxSpiceFmtOnRadioBox )
		self.m_buttonExtract.Bind( wx.EVT_BUTTON, self.m_buttonExtractOnButtonClick )
		self.m_buttonSave.Bind( wx.EVT_BUTTON, self.m_buttonSaveOnButtonClick )
		self.Bind( wx.EVT_TIMER, self.m_timerOnTimer, id=wx.ID_ANY )

	def __del__( self ):
		pass


	# Virtual event handlers, override them in your derived class
	def m_buttonRefreshOnButtonClick( self, event ):
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

	def m_radioBoxSpiceFmtOnRadioBox( self, event ):
		event.Skip()

	def m_buttonExtractOnButtonClick( self, event ):
		event.Skip()

	def m_buttonSaveOnButtonClick( self, event ):
		event.Skip()

	def m_timerOnTimer( self, event ):
		event.Skip()



class z_config_item():
    def __init__(self):
        self.tline = []
        self.ref_net = []
        self.coupled = []
        self.name = "test"
        self.spice_fmt = 0
        self.coupled_max_d = 1
        self.coupled_min_len = 0.2
        

class z_extractor_gui(z_extractor_base):
    def __init__(self):
        z_extractor_base.__init__(self, None)
        self.cfg_list = []
        self.cur_cfg = z_config_item()
        self.net_classes_list = []
        self.board = pcbnew.GetBoard()
        self.sub_process = None
            
        file_name = self.board.GetFileName()
        self.board_path = os.path.split(file_name)[0]
        self.plugin_path = os.path.split(os.path.realpath(__file__))[0]
        file_name = os.path.split(file_name)[1]
        name = file_name.rsplit('.', 1)[0]
        
        self.load_cfg()
        
        self.update_net_classes_ui()
        self.update_cfg_ui()
        self.update_tline_ui()
        self.update_coupled_ui()
        self.update_ref_net_ui()
        self.update_spice_fmt_ui();
        self.Layout()
    
    def load_cfg(self):
        
        cfg_file = open("z_extractor.json", "r")
        if cfg_file == None:
            self.cfg_list[0] = z_config_item()
            self.cur_cfg = self.cfg_list[0]
            return
            
        cfg_list = json.load(cfg_file)
        cfg_file.close()
        
        for cfg in cfg_list:
            item = z_config_item()
            item.name = cfg["name"]
            item.tline = cfg["tline"]
            item.coupled = cfg["coupled"]
            item.ref_net = cfg["ref_net"]
            item.coupled_max_d = cfg["coupled_max_d"]
            item.coupled_min_len = cfg["coupled_min_len"]
            item.spice_fmt = cfg["spice_fmt"]
            
            self.cfg_list.append(item)
        
        if len(self.cfg_list) == 0:
            self.cfg_list[0] = z_config_item()
        
        self.cur_cfg = self.cfg_list[0]
        
    def gen_cmd(self):
        cmd = ""
        for cfg in self.cfg_list:
            cmd = cmd + "kicad_pcb_simulation -pcb " + self.board.GetFileName() + " "
            
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
                
            cmd = cmd + "-coupled_max_d " + str(cfg.coupled_max_d) + " -coupled_min_len " + str(cfg.coupled_min_len) + " "
            if cfg.spice_fmt == 0:
                cmd = cmd + "-ltra 0 "
            elif cfg.spice_fmt == 1:
                cmd = cmd + "-ltra 1 "
                
            cmd = cmd + "-mmtl 1 -o " + cfg.name + ";"
        
        return  cmd.strip(';')
        
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
        self.m_textCtrlMaxDist.SetValue(str(self.cur_cfg.coupled_max_d))
        self.m_textCtrlMinLen.SetValue(str(self.cur_cfg.coupled_min_len))
        
        
    def update_spice_fmt_ui(self):
        #self.m_radioBoxSpiceFmt
        if self.cur_cfg.spice_fmt == 0:
            self.m_radioBoxSpiceFmt.Select(0)
        if self.cur_cfg.spice_fmt == 1:
            self.m_radioBoxSpiceFmt.Select(1)
        
        
    def is_in_coupled(self, net):
        for str in self.cur_cfg.coupled:
            net1 = str.split(":")[0]
            net2 = str.split(":")[1]
            if net == net1 or net == net2:
                return True
        return False
        
    # Virtual event handlers, override them in your derived class
    def m_buttonRefreshOnButtonClick( self, event ):
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
        self.cur_cfg.coupled_min_len = float(self.m_textCtrlMinLen.GetValue())
        event.Skip()

    def m_textCtrlMaxDistOnText( self, event ):
        self.cur_cfg.coupled_max_d = float(self.m_textCtrlMaxDist.GetValue())
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
        

    def m_buttonExtractOnButtonClick( self, event ):
        cmd = self.gen_cmd()
        self.m_textCtrlOutput.AppendText("cmd: "cmd + "\n\n")
        
        #self.m_textCtrlOutput.AppendText(self.board_path + "\n")
        #self.m_textCtrlOutput.AppendText(self.plugin_path + "\n")
        
        self.sub_process = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.board_path, env={"PATH": self.plugin_path})
        
        self.m_buttonExtract.Enable(False)
        self.m_timer.Start(1000)
        
    def m_buttonSaveOnButtonClick( self, event ):
        json_str = json.dumps(self.cur_cfg.__dict__)
        
        list = []
        for cfg in self.cfg_list:
            list.append(cfg.__dict__)
            
        json_str = json.dumps(list)
        
        cfg_file = open("z_extractor.json", "w")
        json.dump(list, cfg_file)
        cfg_file.close()
        
    def m_timerOnTimer( self, event ):
        if self.sub_process.poll() == None:
            self.m_textCtrlOutput.AppendText(".");
        else:
            str = self.sub_process.stdout.read().decode()
            self.m_textCtrlOutput.AppendText("\n" + str + "\n");
            self.m_buttonExtract.Enable()
            self.m_timer.Stop()

        
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
