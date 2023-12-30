#include <linux/limits.h>
#define NO_INCLUDES
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <iterator>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define IMGUI_ENABLE_FREETYPE
#include "imgui/imgui.cpp"
#include "imgui/imgui.h"
#include "imgui/imgui_demo.cpp"
#include "imgui/misc/freetype/imgui_freetype.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/backends/imgui_impl_opengl3_loader.h"
#include "imgui/backends/imgui_impl_glfw.cpp"
#include "imgui/backends/imgui_impl_opengl3.cpp"
#include "ult_globals.h"
#include "ult_common.h"
#include "ult_memory.h"
#include "ult_structs.h"
#include "ult_fileutils.h"
#include "ult_styles.h"

#include "fonts/font-awesome.h"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <unistd.h>

function void
glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

function void
ImportDefaultFont(float BaseFontSize, bool HotReload=0)
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig FontConfig = {};
    FontConfig.SizePixels = BaseFontSize;
    FontConfig.OversampleH = 3;
    FontConfig.OversampleV = 3;
    //io.Fonts->AddFontFromFileTTF("fonts/VT323-Regular.ttf", BaseFontSize, &FontConfig);
    io.Fonts->AddFontDefault(&FontConfig);

    // merge in icons from Font Awesome;
    float iconFontSize = BaseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges );

    if (HotReload)
        ImGui_ImplOpenGL3_CreateFontsTexture();
}

function void
BrowseTo(const char* Path)
{
    char Command[PATH_MAX + 32];
    sprintf(Command, "xdg-open %s", Path);
    fprintf(stdout, "Browsing to %s\n", Path);
    system(Command);
}

function void
BrowseToSubfolder(const char* Path)
{
    char FullPath[PATH_MAX];
    char Cwd[PATH_MAX];
    getcwd(Cwd, sizeof(Cwd));
    sprintf(FullPath, "%s/%s", Cwd, Path);
    BrowseTo(FullPath);
}

function void
RefreshGraphicSettings(GLFWwindow* Window, GLFWmonitor *Monitor, ult_settings* Settings){
    if (Settings->Fullscreen)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(Monitor);
        glfwSetWindowMonitor(Window, Monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
        glViewport(0, 0, mode->width, mode->height);
    }
    else
    {
        glfwSetWindowMonitor(Window, 0, 100, 100, 1280, 720, GLFW_DONT_CARE);
        glViewport(0, 0, 1280, 820);
    }

    if (!Settings->Compositor)
    {
        system("qdbus org.kde.KWin /Compositor resume");
        Settings->Compositor = 1;
    }
    else
    {
        system("qdbus org.kde.KWin /Compositor suspend");
        Settings->Compositor = 0;
    }
}

function void
DisplayWidget(ult_config* Config)
{
    int CurrentDirIndex = 0;
    ult_group** CurrentDir = Config->Groups;
    while (CurrentDirIndex < MAX_ENTRIES) {
        if (*CurrentDir == 0)
            break;

        // Title
        char GroupTitle[NAME_MAX];
        ImGui::AlignTextToFramePadding();
        ImGui::SeparatorText((char *)(*CurrentDir)->GroupTitle.Data);

        // Contents (scripts or applications)
        ult_entry* CurrentEntry = (*CurrentDir)->Entries;
        for (int i = 0; i < (*CurrentDir)->EntriesCount; ++i)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f,0.5f));

            // Generate run button
            if(ImGui::Button((char *)CurrentEntry->EntryTitle.Data, ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f)))
            {
                fprintf(stdout, "Executing action with path: %s\n", (char *)CurrentEntry->Path.Data);
                fprintf(stdout, "Run mode: %d\n", CurrentEntry->RunMode);
                switch (CurrentEntry->RunMode)
                {
                case ult_rm_Browse:
                    BrowseTo((char *)CurrentEntry->Path.Data);
                    break;
                case ult_rm_Script:
                    char command[PATH_MAX + 32];
                    sprintf(command, "konsole -e \"sh %s\" &", (char *)CurrentEntry->Path.Data);
                    system(command);
                    break;
                case ult_rm_Application:
                    pid_t pid = fork();
                    if (pid > 0)
                    {
                        fprintf(stdout, "Launching application using new process...\n");
                    }
                    else if (pid == 0) {
                        fprintf(stdout, "Child process...\n");
                        char command[PATH_MAX + 32];
                        sprintf(command, "%s", (char *)CurrentEntry->Path.Data);
                        int r = execl("/bin/sh", "sh", command, 0);
                        fprintf(stdout, "%d\n", r);
                        if (r != 0)
                        {
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            exit(EXIT_SUCCESS);
                        }

                    }
                    else
                    {
                        fprintf(stdout, "Error creating fork\n");
                    }
                    break;
                }
            }

            ImGui::PopStyleVar(1);
            ++CurrentEntry;
        }

        ImGui::NewLine();
        ++CurrentDirIndex;
        ++CurrentDir;
    }
    ImGui::Separator();
}

function void
LoadConfiguration(ult_state* State)
{
    // Load custom settings
    fileutils_LoadSettings(&State->Settings);

    // Load scripts and applications
    State->ScriptsConfig = fileutils_ExploreSubDirectoriesForConfig(SCRIPTS_DIR, State, ult_rm_Script);
    State->ApplicationsConfig = fileutils_ExploreSubDirectoriesForConfig(APPLICATIONS_DIR, State, ult_rm_Application);

    fileutils_ExploreCustomDirectory(CUSTOM_DIR, State);
}


// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_AUTO_ICONIFIY, GL_FALSE);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "UseLessToolkit", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    GLFWimage images[1];
    images[0].pixels = stbi_load("assets/icon.png", &images[0].width, &images[0].height, 0, 4); //rgba channels
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    SetupImGuiStyle_ClassicSteam();
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Fonts
    int BaseFontSize = 13;
    ImportDefaultFont(BaseFontSize);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // State
    ult_state State = {};
    State.Arena = MakeArena();
    LoadConfiguration(&State);

    // Get compositor state
    char ResultCall[256];
    SystemCall("qdbus org.kde.KWin /Compositor active", ResultCall);
    State.Settings.Compositor = strcmp(ResultCall, "true\n") == 0;
    const char* CompositorLabelOn = ICON_FA_TOGGLE_ON " KDE Compositor";
    const char* CompositorLabelOff =  ICON_FA_TOGGLE_OFF " KDE Compositor";

    // Allocate log
    char* LogBuffer = (char*)MakeLog();
    freopen("/dev/null", "a", stdout);
    setbuf(stdout, LogBuffer);

    // Available screens
    int MonitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&MonitorCount);
    GLFWmonitor* CurrentMonitor = *monitors;

    static bool ReloadConfiguration = 0;
    static bool ForceLayout = 0;
    static bool ReloadFonts = 0;
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        if (ReloadConfiguration)
        {
            ResetArena(&State.Arena);
            LoadConfiguration(&State);
            ReloadConfiguration = 0;
        }

        if (ReloadFonts)
        {
            ImportDefaultFont(BaseFontSize, 1);
            ReloadFonts = 0;
        }

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Menu
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu(ICON_FA_FILE " File"))
            {
                if (ImGui::MenuItem(ICON_FA_FOLDER " Open data directory"))
                {
                    BrowseToSubfolder(DATA_DIR);
                }

                if (ImGui::MenuItem(ICON_FA_ROTATE " Reload Configuration", 0))
                {
                    ReloadConfiguration = 1;
                }
                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FA_DOOR_CLOSED " Quit", 0, false))
                {
                    glfwSetWindowShouldClose(window, 1);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(ICON_FA_GEAR " Options"))
            {
                if (ImGui::BeginMenu(ICON_FA_COMPUTER " Display"))
                {
                    GLFWmonitor** AvailableMonitor = monitors;
                    for (int i = 0; i < MonitorCount; ++i)
                    {
                        char MonitorButton[256];
                        sprintf(MonitorButton, "Fullscreen %d", i + 1);
                        if (ImGui::MenuItem(MonitorButton, 0, State.Settings.Fullscreen && State.Settings.MonitorIndex == i))
                        {
                            State.Settings.Fullscreen = 1;
                            State.Settings.MonitorIndex = i;
                            CurrentMonitor = *AvailableMonitor;
                            RefreshGraphicSettings(window, CurrentMonitor, &State.Settings);
                        }
                        ++AvailableMonitor;
                    }
                    if (ImGui::MenuItem("Windowed", 0, State.Settings.Fullscreen == 0))
                    {
                        State.Settings.Fullscreen = 0;
                        RefreshGraphicSettings(window, CurrentMonitor, &State.Settings);
                    }
                    ImGui::EndMenu();
                }
                ImGui::Separator();

                ImGui::MenuItem(ICON_FA_BULLSEYE " Show KDE settings", 0, &State.Settings.ShowCompositorButton);
                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FA_ANCHOR " Docking", 0, &State.Settings.Dockspace)) {
                    ForceLayout = State.Settings.Dockspace;
                }
                if (ImGui::MenuItem(ICON_FA_BORDER_ALL " Default dock layout", 0, false, State.Settings.Dockspace))
                {
                    ForceLayout = State.Settings.Dockspace;
                }

                ImGui::Separator();
                ImGui::MenuItem(ICON_FA_HOUSE_LAPTOP " Demo window", 0, &State.Settings.ShowWindow[0]);
                ImGui::MenuItem(ICON_FA_PENCIL " Customization window", 0, &State.Settings.ShowWindow[1]);
                ImGui::MenuItem(ICON_FA_CLIPBOARD_QUESTION " Log window", 0, &State.Settings.ShowWindow[2]);
                if (ImGui::MenuItem(ICON_FA_WINDOW_RESTORE " Show all windows", 0, false))
                {
                    state_ShowAllWindows(&State);
                }
                ImGui::EndMenu();
            }

            if (State.Settings.ShowCompositorButton)
            {
                if (ImGui::MenuItem(State.Settings.Compositor ? CompositorLabelOn : CompositorLabelOff, 0))
                {
                    RefreshGraphicSettings(window, CurrentMonitor, &State.Settings);
                }
            }
            ImGui::EndMainMenuBar();
        }

        // Dockspace
        static ImGuiID dockspace_id;
        if (State.Settings.Dockspace)
            dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Build UI
        int WindowIndex = 0;
        if (State.Settings.ShowWindow[WindowIndex])
        {
            ImGui::ShowDemoWindow(&State.Settings.ShowWindow[WindowIndex]);
        }

        ++WindowIndex;
        if (State.Settings.ShowWindow[WindowIndex])
        {
            ImGui::Begin("Customization", &State.Settings.ShowWindow[WindowIndex]);
            ImGui::ColorEdit3("Background color", (float*)&State.Settings.BgColor);
            ImGui::DragFloat("Global font scale", &io.FontGlobalScale, 0.005f, 1.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

            ImGui::SeparatorText("Reimport font");
            ImGui::SliderInt("Font base size", &BaseFontSize, 13, 32);
            if (ImGui::Button("Reimport"))
                ReloadFonts = 1;

            ImGui::ShowFontSelector("Font");

            ImGui::AlignTextToFramePadding();
            ImGui::SeparatorText("Restart to apply following changes");
            ImGui::Checkbox("Force titles upper case", &State.Settings.ForceTitleUpperCase);
            ImGui::Checkbox("Remove file extensions", &State.Settings.RemoveFilesExt);
            ImGui::Checkbox("Show underscores as spaces", &State.Settings.UnderscoresToSpaces);

            if (ImGui::Button("Exit"))
                exit(0);
            ImGui::End();
        }

        ++WindowIndex;
        if (State.Settings.ShowWindow[WindowIndex])
        {
            ImGui::Begin("Log", &State.Settings.ShowWindow[WindowIndex]);

            ImGui::SeparatorText("Log");
            if (ImGui::Button("Copy"))
            {
                ImGui::SetClipboardText(LogBuffer);
            }
            if (ImGui::BeginChild("ult_log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                ImGui::TextUnformatted(LogBuffer);
                ImGui::PopStyleVar();

                // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
                // Using a scrollbar or mouse-wheel will take away from the bottom edge.
                if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }

            }
            ImGui::EndChild();
            ImGui::End();
        }

        ++WindowIndex;
        if (State.Settings.ShowWindow[WindowIndex])
        {
            ImGui::Begin("Scripts", &State.Settings.ShowWindow[WindowIndex]);
            DisplayWidget(State.ScriptsConfig);
            ImGui::End();
        }

        ++WindowIndex;
        if (State.Settings.ShowWindow[WindowIndex])
        {
            ImGui::Begin("Applications", &State.Settings.ShowWindow[WindowIndex]);
            DisplayWidget(State.ApplicationsConfig);
            ImGui::End();
        }

        ult_config* CurrentCustomConfig = State.CustomConfigs;
        for (int i = 0; i <  State.CustomConfigsCount; ++i)
        {
            ++WindowIndex;
            if (State.Settings.ShowWindow[WindowIndex])
            {
                ImGui::Begin((char *)(CurrentCustomConfig)->ConfigTitle.Data, &State.Settings.ShowWindow[WindowIndex]);
                DisplayWidget(CurrentCustomConfig);
                ImGui::End();
            }

            ++CurrentCustomConfig;
        }

        // Default docking
        if (ForceLayout && State.Settings.Dockspace)
        {
            ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
		    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
            const ImVec2 dockspace_size = ImGui::GetContentRegionAvail();
            ImGui::DockBuilderSetNodeSize(dockspace_id, dockspace_size);
            ForceLayout = 0;
        }
        ImGuiDockNode* dock_node = ImGui::DockBuilderGetNode(dockspace_id);
        if (dock_node && State.Settings.Dockspace && !dock_node->IsSplitNode())
        {
            ImGuiID nodeLeft;
            ImGuiID nodeRight;
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.4f, &nodeRight, &nodeLeft);

            ImGuiID nodeLeftUp;
            ImGuiID nodeLeftDown;
            ImGui::DockBuilderSplitNode(nodeLeft, ImGuiDir_Up, 0.5f, &nodeLeftUp, &nodeLeftDown);

            ImGuiID nodeLeftUpA;
            ImGuiID nodeLeftUpB;
            ImGui::DockBuilderSplitNode(nodeLeftUp, ImGuiDir_Right, 0.5f, &nodeLeftUpB, &nodeLeftUpA);

            ImGuiID nodeRightUp;
            ImGuiID nodeRightDown;
            ImGui::DockBuilderSplitNode(nodeRight, ImGuiDir_Up, 0.5f, &nodeRightUp, &nodeRightDown);

            ImGui::DockBuilderDockWindow("Applications", nodeLeftUpA);
            ImGui::DockBuilderDockWindow("Scripts", nodeLeftDown);

            ImGui::DockBuilderDockWindow("Log", nodeRightUp);
            ImGui::DockBuilderDockWindow("Customization", nodeRightDown);

            ult_config* CurrentCustomConfig = State.CustomConfigs;
            for (int i = 0; i <  State.CustomConfigsCount; ++i) {
                ImGui::DockBuilderDockWindow((char *)CurrentCustomConfig->ConfigTitle.Data, nodeLeftUpB);
                ++CurrentCustomConfig;
            }
        }

        // Rendering
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(State.Settings.BgColor.x * State.Settings.BgColor.w,
         State.Settings.BgColor.y * State.Settings.BgColor.w,
         State.Settings.BgColor.z * State.Settings.BgColor.w,
         State.Settings.BgColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    fileutils_SaveSettings(&State.Settings);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
