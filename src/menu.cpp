



#include "menu.h"   
#include "imgui.h" 




void menu::render_menu() {



    ImGui::Begin("my awesome hacky menu");

    ImGui::Text("hello from the injected dll! wassup");
    ImGui::Spacing();

    if (ImGui::Button("click me for fun!")) {


    }

    ImGui::Spacing();


    static bool checkbox_val = false;
    ImGui::Checkbox("toggle something cool", &checkbox_val);
    if (checkbox_val) {
        ImGui::Text("the cool thing is ON!");
    }

    ImGui::Spacing();


    static float slider_val = 0.5f;
    ImGui::SliderFloat("some value adjust", &slider_val, 0.0f, 1.0f);

    ImGui::Spacing();


    static char text_buffer[128] = "type here my dude";
    ImGui::InputText("input field", text_buffer, IM_ARRAYSIZE(text_buffer));

    ImGui::Spacing();
    ImGui::Text("application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);


    ImGui::End();





}
