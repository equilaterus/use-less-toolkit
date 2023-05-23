#define NO_INCLUDES

#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <dirent.h> 
#include <iterator>

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
                    sprintf(command, "konsole -e \"sh scripts/%s/%s\" &", d_path, char_file);
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
                        sprintf(command, "applications/%s/%s", d_path, char_file);
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

        // Customization
        if (ImGui::TreeNode("Customize"))
        {   
            ImGui::Text("Add or edit scripts");
            
            ImGui::SameLine();
            char char_file_run[255];
            sprintf(char_file_run, "Browse-%s", d_path);
            ImGui::PushID(char_file_run);
            if (ImGui::SmallButton("Browse"))
            {
                char char_browse[4096];
                char cwd[PATH_MAX];
                getcwd(cwd, sizeof(cwd));
                sprintf(char_browse, "xdg-open %s/%s/%s", cwd, IsScript ? "scripts" : "applications" ,d_path);
                fprintf(stderr, "%s", char_browse);
                system(char_browse);
            }
            ImGui::PopID();
            ImGui::TreePop();
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
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // State
    ult_state State = {};
    State.Arena = MakeArena();
    // Load scripts and applications
    char pathScripts[256]="scripts";    
    State.Scripts = fileutils_exploreSubDirectoriesForFiles(pathScripts, &State.Arena);
    char pathApplications[256]="applications";    
    State.Applications = fileutils_exploreSubDirectoriesForFiles(pathApplications, &State.Arena);

    State.Settings.ShowSettingsWindow = 1;
    State.Settings.ShowApplicationsWindow = 1;
    State.Settings.ShowScriptsWindow = 1;
    State.Settings.ShowDemoWindow = 1;
    State.Settings.BgColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static bool open_dockspace = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &open_dockspace, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        else
        {
            ShowDockingDisabledMessage();
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Options"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
                ImGui::MenuItem("Padding", NULL, &opt_padding);
                ImGui::Separator();

                if (ImGui::MenuItem("Flag: NoSplit",                "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
                if (ImGui::MenuItem("Flag: NoResize",               "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
                if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
                if (ImGui::MenuItem("Flag: AutoHideTabBar",         "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
                if (ImGui::MenuItem("Flag: PassthruCentralNode",    "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
                ImGui::Separator();

                if (ImGui::MenuItem("Close", NULL, false, open_dockspace))
                    open_dockspace = 0;
                ImGui::EndMenu();
            }
            HelpMarker(
                "When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n"
                "- Drag from window title bar or their tab to dock/undock." "\n"
                "- Drag from window menu button (upper-left button) to undock an entire node (all windows)." "\n"
                "- Hold SHIFT to disable docking (if io.ConfigDockingWithShift == false, default)" "\n"
                "- Hold SHIFT to enable docking (if io.ConfigDockingWithShift == true)" "\n"
                "This demo app has nothing to do with enabling docking!" "\n\n"
                "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window." "\n\n"
                "Read comments in ShowExampleAppDockSpace() for more details.");

            ImGui::EndMenuBar();
        }

        ImGui::End();

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

        

        static bool f = 0;
        if (!f){
            ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
		    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
            f = 1;
            const ImVec2 dockspace_size = ImGui::GetContentRegionAvail();
            ImGui::DockBuilderSetNodeSize(dockspace_id, dockspace_size);
        }
        
        ImGuiDockNode* dock_node = ImGui::DockBuilderGetNode(dockspace_id);
        if (dock_node && !dock_node->IsSplitNode())
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
