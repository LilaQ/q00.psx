#include "ui.h"
#include "cpu.h"
#include "SDL.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_sdlrenderer.h"
#include <iostream>
#include <stdio.h>

namespace UI {

    SDL_WindowFlags window_flags;
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool show_demo_window = true;
    bool showAbout = false;
    bool showCPUregs = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	void init() {

        // Setup window
        window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow("q00.psx[MikeGetFuktEdition]", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

        // Setup SDL_Renderer instance
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            SDL_Log("Error creating SDL_Renderer!");
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer_Init(renderer);

	}

    void draw() {

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if ((event.type == SDL_QUIT) ||
                (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))) {
                // Cleanup
                ImGui_ImplSDLRenderer_Shutdown();
                ImGui_ImplSDL2_Shutdown();
                ImGui::DestroyContext();

                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        //  main menu
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::MenuItem("About")) {
                showAbout = true;
            }
            ImGui::EndMainMenuBar();
        }

        //  cpu regs
        ImGui::Begin("cpu regs", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginTable("cpuregs", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("id");
            ImGui::TableSetupColumn("name");
            ImGui::TableSetupColumn("value");
            ImGui::TableHeadersRow();
            for (int row = 0; row < sizeof(R3000A::registers.r) / sizeof(u32); row++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row);
                ImGui::TableNextColumn();
                ImGui::Text(R3000A::REG_LUT[row]);
                ImGui::TableNextColumn();
                ImGui::Text("0x%08x", R3000A::registers.r[row]);
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("");
            ImGui::TableNextColumn();
            ImGui::Text("pc");
            ImGui::TableNextColumn();
            ImGui::Text("0x%08x", R3000A::registers.pc);

            ImGui::EndTable();
        }
        ImGui::End();

        //  cop0
        ImGui::Begin("cop0", 0, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Columns(2);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "status register");
        if (ImGui::BeginTable("status", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("reg", ImGuiTableColumnFlags_WidthFixed, 170.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoClip, 5.0f); // Default to 200.0f

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("current interrupt enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.current_interrupt_enable);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("current kernel/user mode");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.current_kerneluser_mode);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("prev interrupt enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.prev_interrupt_enable);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("prev kernel/user mode");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.prev_kerneluser_mode);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("old interrupt enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.old_interrupt_enable);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("old kernel/user mode");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.old_kerneluser_mode);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("interrupt mask");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.interrupt_mask);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("isolate cache");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.isolate_cache);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("sweapped cache mode");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.swapped_cache_mode);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("parity zero");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.parity_zero);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cm");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.cm);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cache parity error");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.cache_parity_error);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("tlb shutdown");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.tlb_shutdown);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("boot exception vectors");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.boot_exception_vectors);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("reverse endianness");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.reverse_endianness);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cop0_enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.cop0_enable);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cop1_enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.cop1_enable);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cop2_enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.cop2_enable);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cop3_enable");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].sr.flags.cop3_enable);

            ImGui::EndTable();
        }
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "cause register");
        if (ImGui::BeginTable("cause", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_RowBg))
        {

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("excode");
            ImGui::TableNextColumn();
            ImGui::Text("%s (%02x)", R3000A::COP0_CAUSE_LUT[R3000A::cop[0].cause.excode], R3000A::cop[0].cause.excode);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("interrupt pending");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].cause.interrupt_pending);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("cop id");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].cause.cop_id);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("branch delay");
            ImGui::TableNextColumn();
            ImGui::Text("%x", R3000A::cop[0].cause.branch_delay);

            ImGui::EndTable();
        }
       
        ImGui::End();

        //  about window
        if (showAbout) {
            ImGui::Begin("About", &showAbout);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::TextUnformatted("(((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((/*(((((((((((((\n((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((#%((((((((((((\n(((((((((%((((((((((((((((((((((((((((((((((((((((((((((((((((////##((((((((((((\n((((((((((##(((((((((((((((((((((*/,. .,,/((((((((((((((((((,,,,,(%(((((((#%&&&&\n(((((((((/(//((((((((((((((((((*        ..,,,*,,,////(((/*/**,//(#((((%&&&&&&&&&\n(((((((((/. ..,,,/((((//*.       ....          ,####%%%&&%((///((((%&&&&&&&&&%%&\n((((/(//(//.  .,//(#((#(.           .,       .####%%%%%#((%%(#(((%&&&&&&&&&%&&&&\n&&&&&%***,*,,*((/,,,,,**/*,**.         .,,*//*,,,,*(%%#*,,,,,,/(%&&&&&&&&&&&&&&&\n&&&&&%**,*/&&&&&&&&&&&&&%((%%*,,.,,**,,%&&&&&&&&&&&&&%#%&(****#&&&&&&&&&&&&&&&&&\n#%%%%%&*,,&&&&&&&&&&&&&&&&&&&&,*//*//,#&&&&&&&&&&&&&&&&&&%***%&&&&&&&&&&&&&&&&&&\n&&&&&&&%/*(&&&&&@&&&&&&&&&&&&(*#/.   ,*&&&&&&&&&&&&&&&&&&(*(&&&&&&&&&&&&&&&&&&&&\n&@&&&&&&&&**&&&&&&&&&&&&&&&%,*  ..... .,%&&%&&&&&&&&&&&&,/%&&&&&&&&&&&&&&&&&&&&%\n&&&&&&%@&&&&#*/&&&&&&&&&%#**,,....  ..,,,**&&&&&&&&&&(,(%%&&&&&&&&&&&&%%#(((((((\n%%&&&&&&&&%%&&&&(/******,,.              .  ..,**,*/&&&&&&&&&&&&&&#((((((((/((((\n%%&&&&&&&&&&&&##(,            ..  ,/**//(//((((((/.  ,(##%&&&&%(((/(((((((((((((\n((((((((((((((/ */((((((((/((((/((//(**/,,./(((((/##.  #%%##((((//(//////////(/(\n//(((/(/((/((, /*///((/*(/((*(((/(,(#((/((#//*#(((/%..  (%&&#(////(//////////(/(\n//((/(//(((((  ,/*/,,*/((((((((((((((#((#(./#%(*(#/*...  (%%(/////(//(/(////////\n((((((/((((/*   *((#(,*#(#((#((#(((((####*/###%%#%/,.,..  /%#///(///////////////\n((((/((((///,    .%%%#,/((#(#(((((#(#####(/###%&&&%/*...   ,#///((//////////////\n(((/(///(/(/  .,,/#%%%,,((#(##((#((#####(#,,###(/**,..       ////////(//////////\n///(//////,    ..,.,(*.(((#(#(#(##((#(####///,/(((,..        ,//////////////////\n/////////*     .,//*,./((#####%##%#(######%(((****(*,       .((/(///////////////\n/////////.      ./*,/(((##%#%#%#%%##%(%#%%%%###((#/(,       ,%#/((//////////////\n//////(/(,       *,#(#(#%#%%%%%%#(###%%%#%%%%##(#(/,.       ./##////////////////\n/////////,        ,/((#%###%##&%&#%%#%%%%&#%%#%#((,.....  ,,(%&%((///(//////////\n/(////////        ..*%%%%%%%%%&%%%%&%%%%%%%###%%%#(/#%(&%&&&%%&(((//////////////\n///////((//#((/(((/***/((//(/*/*****,****,,,*,,,*/##%%#%&&&&&,,.////////////////\n//((/////(/((((((*/.       ....,.,,,**,,.,.,,...*/(%%&%&&&#&&&&/..*&#,,,*///////\n/////////(////(*//*/*.    ..,,***//(*(((#(#(#%#&(%#&&&&&&&&&&&&&@,.,(&(*,,*/////\n///////////////////(/**,,.  ..,....,/((&#&%%&#&&&%%&&&&&&&&&&&&&&&#.,*&&&&#/////\n//////////////////////(///*/ .,**/(%(#%%%&&&&&&&&&&%*/(&&&&&&&&&&&&&(,,%&&&&%///");
            if (ImGui::Button("Eat shit"))
                showAbout = false;
            ImGui::End();
        }

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::Begin("BottomBar");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

    }
}