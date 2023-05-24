#define NO_INCLUDES
#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <dirent.h> 
#include <iterator>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"


#include "imgui/imgui.cpp"
#include "imgui/imgui.h"
#include "imgui/imgui_demo.cpp"
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

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <unistd.h>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

function void
BrowseTo(const char* Path)
{
    char char_browse[4096];
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    sprintf(char_browse, "xdg-open %s/%s", cwd, Path);
    fprintf(stderr, "Browse to %s", char_browse);
    system(char_browse);
}

function void
RefreshGraphicSettings(GLFWwindow* Window, GLFWmonitor *Monitor, ult_settings* Settings){
    if (Settings->Fullscreen)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(Monitor);
        glfwSetWindowMonitor(Window, Monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
        glViewport(0, 0, mode->width, mode->height);
    }
    else {
        glfwSetWindowMonitor(Window, 0, 100, 100, 1280, 720, GLFW_DONT_CARE);
        glViewport(0, 0, 1280, 820);
    }

    if (Settings->Compositor){
        system("qdbus org.kde.KWin /Compositor resume");
    }
    else {
        system("qdbus org.kde.KWin /Compositor suspend");
    }
}

function void
DisplayDirectory(subdirectories* Subdirectories, bool IsScript)
{
    int CurrentDirIndex = 0;
    directory_contents** CurrentDir = Subdirectories->Directories;
    while (CurrentDirIndex < MAX_DIRS)
    {
        char d_path[255];
        if (*CurrentDir == 0)
        {
            break;
        }
        // Title
        StringToChar(&(*CurrentDir)->Name, d_path);
        ImGui::AlignTextToFramePadding();
        ImGui::SeparatorText(d_path);

        // Contents (scripts or applications)
        string* CurrentFile = (*CurrentDir)->Files;
        for (int i = 0; i < (*CurrentDir)->FilesCount; ++i) 
        {
            char char_file[255];
            StringToChar(CurrentFile, char_file);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f,0.5f));
            if (IsScript)
            {                
                if(ImGui::Button(char_file, ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f)))
                {
                    char command[255];
                    sprintf(command, "konsole -e \"sh %s/%s/%s\" &", SCRIPTS_DIR, d_path, char_file);
                    system(command);
                }
            }
            else
            {
                char char_file_run[255];
                sprintf(char_file_run, "Run %s", char_file);
                if(ImGui::Button(char_file_run,  ImVec2(ImGui::GetContentRegionAvail().x * 1.0f, 0.0f)))
                {
                    pid_t pid = fork();
                    if (pid > 0) {
                        fprintf(stderr, "Launching application using new process...\n");
                    } else if (pid == 0) {
                        fprintf(stderr, "Child process...\n");                        
                        char command[255];
                        sprintf(command, "%s/%s/%s", APPLICATIONS_DIR, d_path, char_file);
                        int r = execl("/bin/sh", "sh", command, 0);
                        fprintf(stderr, "%d\n", r);
                        if (r != 0) {
                            exit(EXIT_FAILURE);
                        } else {
                            exit(EXIT_SUCCESS);
                        }
                        
                    } else {
                        fprintf(stderr, "Error creating fork\n");
                    }
                }            
            }
            ImGui::PopStyleVar(1);
            ++CurrentFile;
        }

        ImGui::NewLine();
        ++CurrentDirIndex;
        ++CurrentDir;
    }
    ImGui::Separator();
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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    SetupImGuiStyle_ClassicSteam();
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // State
    ult_state State = {};
    State.Arena = MakeArena();
    // Load scripts and applications
    State.Scripts = fileutils_exploreSubDirectoriesForFiles(SCRIPTS_DIR, &State.Arena);
    State.Applications = fileutils_exploreSubDirectoriesForFiles(APPLICATIONS_DIR, &State.Arena);

    State.Settings.Fullscreen = 0;
    char ResultCall[256];
    SystemCall("qdbus org.kde.KWin /Compositor active", ResultCall);
    State.Settings.Compositor = strcmp(ResultCall, "true");
    State.Settings.Dockspace = 1;
    State.Settings.ShowSettingsWindow = 1;
    State.Settings.ShowApplicationsWindow = 1;
    State.Settings.ShowScriptsWindow = 1;
    State.Settings.ShowDemoWindow = 1;
    State.Settings.BgColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    int MonitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&MonitorCount);
    GLFWmonitor* CurrentMonitor = *monitors;

    static bool ForceLayout = 1;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Dockspace
        static ImGuiID dockspace_id;        
        if (ImGui::BeginMainMenuBar())
        { 
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::BeginMenu("Open directory"))
                {
                    if (ImGui::MenuItem("data/"))
                    {
                        BrowseTo(DATA_DIR);
                    }
                    if (ImGui::MenuItem("applications/"))
                    {
                        BrowseTo(APPLICATIONS_DIR);
                    }
                    if (ImGui::MenuItem("scripts/"))
                    {
                        BrowseTo(SCRIPTS_DIR);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Quit", 0, false))
                {
                    exit(0);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options"))
            {
                if (ImGui::BeginMenu("Display"))
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

                if (ImGui::MenuItem("Compositor", 0, &State.Settings.Compositor))
                {
                    RefreshGraphicSettings(window, CurrentMonitor, &State.Settings);
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Docking", 0, &State.Settings.Dockspace))
                {
                    ForceLayout = State.Settings.Dockspace;
                }
                if (ImGui::MenuItem("Default dock layout", 0, false, State.Settings.Dockspace))
                {
                    ForceLayout = State.Settings.Dockspace;
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        
        if (State.Settings.Dockspace)
            dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        
        // Build UI
        if (State.Settings.ShowDemoWindow)
           ImGui::ShowDemoWindow(&State.Settings.ShowDemoWindow);

        if (State.Settings.ShowSettingsWindow)
        {
            ImGui::Begin("Settings", &State.Settings.ShowSettingsWindow);
            ImGui::ColorEdit3("Background color", (float*)&State.Settings.BgColor);
            ImGui::End();
        }

        if (State.Settings.ShowScriptsWindow)
        {
            ImGui::Begin("Scripts", &State.Settings.ShowScriptsWindow);       
            DisplayDirectory(State.Scripts, 1);
            ImGui::End();
        }

        if (State.Settings.ShowApplicationsWindow)
        {
            ImGui::Begin("Applications", &State.Settings.ShowApplicationsWindow);            
            DisplayDirectory(State.Applications, 0);
            ImGui::End();
        }

        if (ForceLayout && State.Settings.Dockspace){
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

            ImGuiID nodeRightUp;
            ImGuiID nodeRightDown;
            ImGui::DockBuilderSplitNode(nodeRight, ImGuiDir_Up, 0.5f, &nodeRightUp, &nodeRightDown);

            ImGui::DockBuilderDockWindow("Applications", nodeLeftUp);
            ImGui::DockBuilderDockWindow("Scripts", nodeLeftDown);

            ImGui::DockBuilderDockWindow("Settings", nodeRightUp);
            ImGui::DockBuilderDockWindow("Dear ImGui Demo", nodeRightDown);
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

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
