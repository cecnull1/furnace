//
// Created by Administrator on 2026/9/19.
//

#include "CSE_INV.h"
#include <imgui.h>

#include "../../../gui/gui.h"
#include "../../../engine/platform/sound/cse1/cse1.hpp"

#define BEGIN_TAB_ITEM(s) if (ImGui::BeginTabItem(s))
#define END_TAB_ITEM ImGui::EndTabItem();


constexpr auto gen_min = 0;
constexpr auto gen_max = 0xffff;
constexpr CSE1_PACKED::CSE1_DOUBLE_REG big_max = 0x7fffffffu;
constexpr auto fourBit_max = 0x0f;
constexpr auto threeBit_max = 0x07;
constexpr auto sixBit_max = 0x3f;
constexpr auto twoBit_max = 0x03;

void FurnaceGUI::drawInsCSE1(DivInstrument *ins) {
    auto& cse1 = ins -> cse1;
    BEGIN_TAB_ITEM("CSE-1") {
        if (ImGui::BeginTable("Table", 4, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("OP");
            ImGui::TableSetupColumn("Control");
            ImGui::TableSetupColumn("ADSR");
            ImGui::TableSetupColumn("");
            ImGui::TableHeadersRow();
            for (int i = 0; i < CSE1_OPER_NUMBER; i++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("OP%d", i+1);
                ImGui::PushID(i);
                for (int j = 0; j < CSE1_OPER_NUMBER; j++) {
                    ImGui::PushID(j);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::SliderScalar(
                    "##MI",
                    ImGuiDataType_U16,
                    &cse1.op[i].mi[j],
                    &gen_min, &gen_max,
                    fmt::format("MI{0}: %d", j + 1).c_str()
                    );
                    ImGui::PopID();
                }

                ImGui::TableNextColumn();
                ImGui::NewLine();

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##PHASE",
                ImGuiDataType_S32,
                &cse1.op[i].phase,
                &gen_min, &big_max,
                "PHASE: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##ML",
                ImGuiDataType_U8,
                &cse1.op[i].ml,
                &gen_min, &fourBit_max,
                "ML: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##OPN2DT",
                ImGuiDataType_U8,
                &cse1.op[i].dn,
                &gen_min, &threeBit_max,
                "OPN2DT: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##ESTA",
                ImGuiDataType_U8,
                &cse1.op[i].adsr.adsrState,
                &gen_min, &twoBit_max,
                "ESTA: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##WAVE",
                ImGuiDataType_U8,
                &cse1.op[i].wave,
                &gen_min, &threeBit_max,
                "WAVE: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##DUTY",
                ImGuiDataType_U16,
                &cse1.op[i].duty,
                &gen_min, &gen_max,
                "DUTY: %d"
                );

                ImGui::TableNextColumn();
                ImGui::NewLine();

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##AR",
                ImGuiDataType_U16,
                &cse1.op[i].adsr.ar,
                &gen_min, &big_max,
                "A: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##DR",
                ImGuiDataType_U16,
                &cse1.op[i].adsr.dr,
                &gen_min, &big_max,
                "D: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##SR",
                ImGuiDataType_U16,
                &cse1.op[i].adsr.sr,
                &gen_min, &big_max,
                "D2: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##SL",
                ImGuiDataType_U16,
                &cse1.op[i].adsr.sl,
                &gen_min, &big_max,
                "S: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##RR",
                ImGuiDataType_U16,
                &cse1.op[i].adsr.rr,
                &gen_min, &big_max,
                "R: %d"
                );

                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::SliderScalar(
                "##EDIV",
                ImGuiDataType_U8,
                &cse1.op[i].env_divider,
                &gen_min, &sixBit_max,
                "EDIV: %d"
                );

                ImGui::TableNextColumn();

                ImVec2 sliderSize=ImVec2(dpiScale,dpiScale);

                drawCSE1Env(gen_min,
                    cse1.op[i].adsr.ar,
                    cse1.op[i].adsr.dr,
                    cse1.op[i].adsr.sr,
                    cse1.op[i].adsr.rr,
                    cse1.op[i].adsr.sl,
                    gen_max,
                    gen_min,
                    gen_min,
                    gen_max,
                    gen_max,
                    gen_max,
                    ImVec2(ImGui::GetContentRegionAvail().x,200*sliderSize.y),
                    ins->type);

                ImGui::PopID();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("OUT");
            ImGui::PushID("OUT");

            for (int j = 0; j < CSE1_OPER_NUMBER; j++) {
                ImGui::PushID(j);
                const float halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
                ImGui::SetNextItemWidth(halfWidth);
                ImGui::SliderScalar("##IN_LEFT",
                ImGuiDataType_U16,
                &cse1.out.inLeft[j],
                &gen_min, &gen_max,
                fmt::format("INL{0}: %d", j + 1).c_str());

                ImGui::SameLine();

                ImGui::SetNextItemWidth(halfWidth);
                ImGui::SliderScalar("##IN_RIGHT",
                ImGuiDataType_U16,
                &cse1.out.inRight[j],
                &gen_min, &gen_max,
                fmt::format("INR{0}: %d", j + 1).c_str());
                ImGui::PopID();
            }

            ImGui::PushID(CSE1_OPER_NUMBER+1);
            const float halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;
            ImGui::SetNextItemWidth(halfWidth);
            ImGui::SliderScalar("##OUT_LEFT",
            ImGuiDataType_U16,
            &cse1.out.outLeft,
            &gen_min, &gen_max, "OUTL: %d");

            ImGui::SameLine();

            ImGui::SetNextItemWidth(halfWidth);
            ImGui::SliderScalar("##OUT_RIGHT",
                    ImGuiDataType_U16,
                    &cse1.out.outRight,
                    &gen_min, &gen_max, "OUTR: %d");
            ImGui::PopID();

            ImGui::PopID();
            ImGui::EndTable();
        }
        END_TAB_ITEM
    }
}
