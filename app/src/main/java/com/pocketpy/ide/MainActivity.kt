package com.pocketpy.ide

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.pocketpy.ide.ui.drawer.WorkspaceDrawer
import com.pocketpy.ide.ui.editor.CodeEditorView
import com.pocketpy.ide.ui.terminal.TerminalView
import com.pocketpy.ide.ui.theme.*
import com.pocketpy.ide.ui.viewmodel.MainViewModel
import com.pocketpy.ide.ui.visual.VisualWebView
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    private val viewModel: MainViewModel by viewModels()

    @OptIn(ExperimentalMaterial3Api::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            PocketPyTheme {
                val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
                val scope = rememberCoroutineScope()

                val files by viewModel.files.collectAsState()
                val activeFile by viewModel.activeFile.collectAsState()
                val codeState by viewModel.codeState.collectAsState()
                val terminalOutput by viewModel.terminalOutput.collectAsState()
                val htmlOutput by viewModel.htmlOutput.collectAsState()
                val isRunning by viewModel.isRunning.collectAsState()
                val selectedTab by viewModel.selectedTab.collectAsState()

                ModalNavigationDrawer(
                    drawerState = drawerState,
                    drawerContent = {
                        WorkspaceDrawer(
                            files = files,
                            activeFile = activeFile,
                            onSelectFile = { viewModel.selectFile(it) },
                            onCreateFile = { viewModel.createFile(it) },
                            onRenameFile = { file, name -> viewModel.renameFile(file, name) },
                            onDeleteFile = { viewModel.deleteFile(it) },
                            onRestoreSamples = { viewModel.restoreSamples() },
                            onCloseDrawer = { scope.launch { drawerState.close() } }
                        )
                    }
                ) {
                    Scaffold(
                        topBar = {
                            TopAppBar(
                                title = {
                                    Column {
                                        Text(
                                            text = activeFile?.name ?: "PocketPy IDE",
                                            color = IdeTextPrimary,
                                            fontWeight = FontWeight.Bold,
                                            fontSize = 16.sp,
                                            fontFamily = FontFamily.Monospace
                                        )
                                        Text(
                                            text = "PocketPy (C11 Engine)",
                                            color = IdeTextMuted,
                                            fontSize = 11.sp
                                        )
                                    }
                                },
                                navigationIcon = {
                                    IconButton(onClick = { scope.launch { drawerState.open() } }) {
                                        Icon(Icons.Default.Menu, contentDescription = "Menu", tint = IdeTextPrimary)
                                    }
                                },
                                actions = {
                                    // Save Button
                                    IconButton(onClick = { viewModel.saveCurrentFile() }) {
                                        Icon(Icons.Default.Save, contentDescription = "Save", tint = IdeTextSecondary)
                                    }
                                    // Run / Stop Button
                                    if (isRunning) {
                                        Button(
                                            onClick = { viewModel.stopScript() },
                                            colors = ButtonDefaults.buttonColors(containerColor = IdeError)
                                        ) {
                                            Icon(Icons.Default.Stop, contentDescription = null, modifier = Modifier.size(16.dp))
                                            Spacer(modifier = Modifier.width(4.dp))
                                            Text("Stop", fontWeight = FontWeight.Bold)
                                        }
                                    } else {
                                        Button(
                                            onClick = { viewModel.runScript() },
                                            colors = ButtonDefaults.buttonColors(containerColor = IdePrimary, contentColor = IdeBackground)
                                        ) {
                                            Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(16.dp))
                                            Spacer(modifier = Modifier.width(4.dp))
                                            Text("Run", fontWeight = FontWeight.Bold)
                                        }
                                    }
                                },
                                colors = TopAppBarDefaults.topAppBarColors(containerColor = IdeSurface)
                            )
                        },
                        bottomBar = {
                            NavigationBar(
                                containerColor = IdeSurface,
                                contentColor = IdeTextPrimary
                            ) {
                                NavigationBarItem(
                                    selected = selectedTab == 0,
                                    onClick = { viewModel.selectTab(0) },
                                    icon = { Icon(Icons.Default.Edit, contentDescription = "Editor") },
                                    label = { Text("Editor") },
                                    colors = NavigationBarItemDefaults.colors(
                                        selectedIconColor = IdePrimary,
                                        selectedTextColor = IdePrimary,
                                        indicatorColor = IdeSurfaceVariant
                                    )
                                )
                                NavigationBarItem(
                                    selected = selectedTab == 1,
                                    onClick = { viewModel.selectTab(1) },
                                    icon = { Icon(Icons.Default.Terminal, contentDescription = "Terminal") },
                                    label = { Text("Terminal") },
                                    colors = NavigationBarItemDefaults.colors(
                                        selectedIconColor = IdePrimary,
                                        selectedTextColor = IdePrimary,
                                        indicatorColor = IdeSurfaceVariant
                                    )
                                )
                                NavigationBarItem(
                                    selected = selectedTab == 2,
                                    onClick = { viewModel.selectTab(2) },
                                    icon = { Icon(Icons.Default.Language, contentDescription = "Visual") },
                                    label = { Text("Visual") },
                                    colors = NavigationBarItemDefaults.colors(
                                        selectedIconColor = IdePrimary,
                                        selectedTextColor = IdePrimary,
                                        indicatorColor = IdeSurfaceVariant
                                    )
                                )
                            }
                        }
                    ) { innerPadding ->
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(innerPadding)
                                .background(IdeBackground)
                        ) {
                            when (selectedTab) {
                                0 -> CodeEditorView(
                                    textFieldValue = codeState,
                                    onValueChange = { viewModel.onCodeChange(it) }
                                )
                                1 -> TerminalView(
                                    output = terminalOutput,
                                    isRunning = isRunning,
                                    onStop = { viewModel.stopScript() },
                                    onClear = { viewModel.clearTerminal() }
                                )
                                2 -> VisualWebView(
                                    htmlContent = htmlOutput,
                                    onClearVisual = { viewModel.clearVisual() }
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
