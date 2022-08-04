#!/usr/bin/env python

import pcbnew
import os
import stat
import wx
import subprocess
import json
import time
import platform


###########################################################################
## Class rl_extractor_base
###########################################################################

class rl_extractor_base ( wx.Dialog ):

	def __init__( self, parent ):
		wx.Dialog.__init__ ( self, parent, id = wx.ID_ANY, title = u"rl_extractor", pos = wx.DefaultPosition, size = wx.Size( 900,640 ), style = wx.DEFAULT_DIALOG_STYLE|wx.MAXIMIZE_BOX|wx.RESIZE_BORDER )

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

		sbSizer7 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Pads" ), wx.VERTICAL )

		m_listBoxPadsChoices = []
		self.m_listBoxPads = wx.ListBox( sbSizer7.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxPadsChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer7.Add( self.m_listBoxPads, 1, wx.EXPAND, 5 )

		bSizer9 = wx.BoxSizer( wx.VERTICAL )


		sbSizer7.Add( bSizer9, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer7, 1, wx.EXPAND, 5 )

		sbSizer3 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Pad-Pad" ), wx.VERTICAL )

		m_listBoxPadToPadChoices = []
		self.m_listBoxPadToPad = wx.ListBox( sbSizer3.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxPadToPadChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer3.Add( self.m_listBoxPadToPad, 1, wx.EXPAND, 5 )

		bSizer61 = wx.BoxSizer( wx.HORIZONTAL )

		self.m_buttonPadAdd = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonPadAdd, 1, wx.EXPAND, 5 )

		self.m_buttonPadDel = wx.Button( sbSizer3.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer61.Add( self.m_buttonPadDel, 1, wx.EXPAND, 5 )


		sbSizer3.Add( bSizer61, 0, wx.EXPAND, 5 )


		bSizer1.Add( sbSizer3, 1, wx.EXPAND, 5 )

		sbSizer2 = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Net" ), wx.VERTICAL )

		m_listBoxNetChoices = []
		self.m_listBoxNet = wx.ListBox( sbSizer2.GetStaticBox(), wx.ID_ANY, wx.DefaultPosition, wx.DefaultSize, m_listBoxNetChoices, wx.LB_EXTENDED|wx.LB_HSCROLL )
		sbSizer2.Add( self.m_listBoxNet, 1, wx.EXPAND, 5 )

		bSizer10 = wx.BoxSizer( wx.VERTICAL )

		self.m_buttonNetAdd = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"add", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer10.Add( self.m_buttonNetAdd, 0, wx.EXPAND, 5 )

		self.m_buttonNetDel = wx.Button( sbSizer2.GetStaticBox(), wx.ID_ANY, u"delete", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer10.Add( self.m_buttonNetDel, 0, wx.EXPAND, 5 )


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

		bSizer8 = wx.BoxSizer( wx.HORIZONTAL )


		bSizer8.Add( ( 0, 0), 1, wx.EXPAND, 5 )

		self.m_buttonExtract = wx.Button( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Extract", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer8.Add( self.m_buttonExtract, 0, wx.ALIGN_CENTER|wx.ALL, 5 )

		self.m_checkBoxExtractAll = wx.CheckBox( sbSizer6.GetStaticBox(), wx.ID_ANY, u"Extract All", wx.DefaultPosition, wx.DefaultSize, 0 )
		bSizer8.Add( self.m_checkBoxExtractAll, 0, wx.ALIGN_CENTER|wx.ALL, 5 )


		sbSizer6.Add( bSizer8, 1, wx.ALL|wx.EXPAND, 5 )


		bSizer4.Add( sbSizer6, 0, wx.ALIGN_CENTER|wx.EXPAND, 5 )

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
		self.m_buttonPadAdd.Bind( wx.EVT_BUTTON, self.m_buttonPadAddOnButtonClick )
		self.m_buttonPadDel.Bind( wx.EVT_BUTTON, self.m_buttonPadDelOnButtonClick )
		self.m_buttonNetAdd.Bind( wx.EVT_BUTTON, self.m_buttonNetAddOnButtonClick )
		self.m_buttonNetDel.Bind( wx.EVT_BUTTON, self.m_buttonNetDelOnButtonClick )
		self.m_listBoxCfg.Bind( wx.EVT_LISTBOX, self.m_listBoxCfgOnListBox )
		self.m_buttonCfgAdd.Bind( wx.EVT_BUTTON, self.m_buttonCfgAddOnButtonClick )
		self.m_buttonCfgDel.Bind( wx.EVT_BUTTON, self.m_buttonCfgDelOnButtonClick )
		self.m_buttonCfgRename.Bind( wx.EVT_BUTTON, self.m_buttonCfgRenameOnButtonClick )
		self.m_buttonSave.Bind( wx.EVT_BUTTON, self.m_buttonSaveOnButtonClick )
		self.m_buttonExtract.Bind( wx.EVT_BUTTON, self.m_buttonExtractOnButtonClick )
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

	def m_buttonPadAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonPadDelOnButtonClick( self, event ):
		event.Skip()

	def m_buttonNetAddOnButtonClick( self, event ):
		event.Skip()

	def m_buttonNetDelOnButtonClick( self, event ):
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

	def m_buttonExtractOnButtonClick( self, event ):
		event.Skip()

	def m_timerOnTimer( self, event ):
		event.Skip()


class rl_config_item():
    def __init__(self):
        self.rl_net = []
        self.pad2pad = []
        self.name = "newcfg"
        

class rl_extractor_gui(rl_extractor_base):
    def __init__(self):
        rl_extractor_base.__init__(self, None)
        self.cfg_list = []
        self.cur_cfg = rl_config_item()
        self.net_classes_list = []
        self.board = pcbnew.GetBoard()
        self.sub_process = None
        self.start_time = time.perf_counter()
            
        file_name = self.board.GetFileName()
        self.board_path = os.path.split(file_name)[0]
        self.plugin_path = os.path.split(os.path.realpath(__file__))[0]
        
        self.output_path = self.board_path + os.sep + "rl_extractor"
        if not os.path.exists(self.output_path):
            os.mkdir(self.output_path)
        
        sys = platform.system()
        if sys == "Windows":
            self.plugin_bin_path = self.plugin_path + os.sep + "win"
        elif sys == "Linux":
            self.plugin_bin_path = self.plugin_path + os.sep + "linux"
            os.chmod(self.plugin_path + os.sep + "linux" + os.sep + "fasthenry", stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP)
            os.chmod(self.plugin_path + os.sep + "linux" + os.sep + "z_extractor", stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP)
        else:
            pass
        
        self.load_cfg()
        
        self.update_net_classes_ui()
        self.update_cfg_ui()
        self.update_pads_ui()
        self.update_net_ui()
        self.update_pad2pad_ui()
    
    def load_cfg(self):
        
        try:
            cfg_file = open(self.board_path + os.sep + "rl_extractor.json", "r")
        except:
            self.cfg_list.append(rl_config_item())
            self.cur_cfg = self.cfg_list[0]
            return
            
        cfg_list = json.load(cfg_file)
        cfg_file.close()
        
        for cfg in cfg_list:
            item = rl_config_item()
            if "name" in cfg:
                item.name = cfg["name"]
            if "rl_net" in cfg:
                item.rl_net = cfg["rl_net"]
            if "pad2pad" in cfg:
                item.pad2pad = cfg["pad2pad"]
            
            self.cfg_list.append(item)
        
        if len(self.cfg_list) == 0:
            self.cfg_list.append(rl_config_item())
        
        self.cur_cfg = self.cfg_list[0]
        
    def gen_cmd(self, all = True):
        cmd = ""
        if all:
            cfg_list = self.cfg_list
        else:
            cfg_list = [self.cur_cfg]
            
        for cfg in cfg_list:
            cmd = cmd + "z_extractor -pcb " + self.board.GetFileName() + " -rl "
            
            if len(cfg.rl_net) > 0:
                cmd = cmd + '-net "'
                for rl_net in cfg.rl_net:
                    cmd = cmd + rl_net + ","
                cmd = cmd.strip(',') + '" '
                
            if len(cfg.pad2pad) > 0:
                cmd = cmd + '-pad "'
                for pad2pad in cfg.pad2pad:
                    cmd = cmd + pad2pad + ","
                cmd = cmd.strip(',') + '" '
                
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
    
    def update_pads_ui(self):
        selected = self.m_listBoxNetClasses.GetSelections()
        if len(selected) == 0:
            return
            
        self.m_listBoxPads.Clear()
        net_name = self.m_listBoxNetClasses.GetString(selected[0])
            
        for m in self.board.GetFootprints():
            footprint = m.GetReference()
            for p in m.Pads():
                if p.GetNet().GetNetname() == net_name:
                    self.m_listBoxPads.Append(footprint + "." + p.GetName())
            
    
        
    def update_cfg_ui(self):
        self.m_listBoxCfg.Clear()
        for cfg in self.cfg_list:
            self.m_listBoxCfg.Append(cfg.name)
        
        self.m_listBoxCfg.SetStringSelection(self.cur_cfg.name)
    
    def update_net_ui(self):
        self.m_listBoxNet.Clear()
        for net in self.cur_cfg.rl_net:
            self.m_listBoxNet.Append(net)
            
    def update_pad2pad_ui(self):
        self.m_listBoxPadToPad.Clear()
        for net in self.cur_cfg.pad2pad:
            self.m_listBoxPadToPad.Append(net)
        
    # Virtual event handlers, override them in your derived class
    def m_listBoxNetClassesOnListBox( self, event ):
        self.update_pads_ui()
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
            if (net not in self.cur_cfg.rl_net):
                self.cur_cfg.rl_net.append(net)
                self.m_listBoxNet.Append(net)
        
    def m_buttonNetDelOnButtonClick( self, event ):
        selected = self.m_listBoxNet.GetSelections()
        size = len(selected)
        while size > 0:
            size = size - 1
            net = self.m_listBoxNet.GetString(selected[size])
            self.m_listBoxNet.Delete(selected[size])
            self.cur_cfg.rl_net.remove(net)
        
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
        self.update_pads_ui()
        self.update_net_ui()
        self.update_pad2pad_ui()
        
    def m_buttonCfgAddOnButtonClick( self, event ):
        dlg = wx.TextEntryDialog(None, u"cfg name", u"name:")
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if name != "" and self.m_listBoxCfg.FindString(name) < 0:
                cfg = rl_config_item()
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
        self.update_pads_ui()
        self.update_net_ui()
        self.update_pad2pad_ui()
        
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

    def m_buttonExtractOnButtonClick( self, event ):
        if self.m_buttonExtract.GetLabel() == "Terminate":
            if self.sub_process.poll() == None:
                self.sub_process.terminate()
            self.m_buttonExtract.SetLabel("Extract")
            self.m_timer.Stop()
            return
            
        cmd = self.gen_cmd(self.m_checkBoxExtractAll.GetValue())
        self.m_textCtrlOutput.AppendText("cmd: " + cmd + "\n\n")
        self.start_time = time.perf_counter()
        self.sub_process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, cwd=self.output_path, env={"PATH": self.plugin_bin_path})
        
        self.m_buttonExtract.SetLabel("Terminate")
        self.m_timer.Start(1000)
        
    def m_buttonSaveOnButtonClick( self, event ):
        json_str = json.dumps(self.cur_cfg.__dict__)
        
        list = []
        for cfg in self.cfg_list:
            list.append(cfg.__dict__)
            
        json_str = json.dumps(list)
        
        cfg_file = open(self.board_path + os.sep + "rl_extractor.json", "w")
        json.dump(list, cfg_file)
        cfg_file.close()
        
    def m_timerOnTimer( self, event ):
        if self.sub_process.poll() == None:
            self.m_textCtrlOutput.AppendText(".");
        else:
            str = self.sub_process.stdout.read().decode()
            self.m_textCtrlOutput.AppendText("\n" + str + "\n");
            self.m_textCtrlOutput.AppendText("time: {:.3f}s\n".format((time.perf_counter() - self.start_time)));
            self.m_buttonExtract.SetLabel("Extract")
            self.m_timer.Stop()
            self.sub_process.wait(0.1)

        
class rl_extractor( pcbnew.ActionPlugin ):
    def defaults( self ):
        self.name = "RL Extractor"
        self.category = "RL Extractor"
        self.description = "RL Extraction"
        self.show_toolbar_button = True
        #self.icon_file_name = os.path.join(os.path.dirname(__file__), 'icon.png')

    def Run( self ):
        dig = rl_extractor_gui()
        dig.Show()
        
rl_extractor().register()
