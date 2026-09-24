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
constexpr auto byteBit_max = 0xff;
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
                &gen_min, &byteBit_max,
                "EDIV: %d"
                );

                ImGui::TableNextColumn();

                ImVec2 sliderSize=ImVec2(dpiScale,dpiScale);

                drawCSE1Env(gen_min,
                    cse1.op[i].adsr.ar,
                    cse1.op[i].adsr.dr,
                    cse1.op[i].adsr.sr,
                    cse1.op[i].adsr.rr,
                    65535-cse1.op[i].adsr.sl,
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

#include "../insEditCommon.h"

void FurnaceGUI::drawCSE1Env(uint16_t tl, uint16_t ar, uint16_t dr, uint16_t d2r, uint16_t rr, uint16_t sl, uint16_t sus, uint16_t egt, uint16_t algOrGlobalSus, float maxTl, float maxArDr, float maxRr, const ImVec2& size, unsigned short instType) {
  ImDrawList* dl=ImGui::GetWindowDrawList();
  ImGuiWindow* window=ImGui::GetCurrentWindow();

  ImVec2 minArea=window->DC.CursorPos;
  ImVec2 maxArea=ImVec2(
    minArea.x+size.x,
    minArea.y+size.y
  );
  ImRect rect=ImRect(minArea,maxArea);
  ImGuiStyle& style=ImGui::GetStyle();
  ImU32 color=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE]);
  ImU32 colorR=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_RELEASE]); // Relsease triangle
  ImU32 colorS=ImGui::GetColorU32(uiColors[GUI_COLOR_FM_ENVELOPE_SUS_GUIDE]); // Sustain horiz/vert line color
  ImGui::ItemSize(size,style.FramePadding.y);
  if (ImGui::ItemAdd(rect,ImGui::GetID("fmEnv"))) {
    ImGui::RenderFrame(rect.Min,rect.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),true,style.FrameRounding);

    // Adjust for OPLL global sustain setting
    if (instType==DIV_INS_OPLL && algOrGlobalSus==1.0) {
      rr=5.0;
    }
    // calculate x positions
    float arPos=float(maxArDr-(float)ar)/maxArDr; // peak of AR, start of DR
    float drPos=arPos+(((float)sl/65535.0)*(float(maxArDr-(float)dr)/maxArDr)); // end of DR, start of D2R
    float d2rPos=drPos+(((65535.0-(float)sl)/65535.0)*(float(65535.0-(float)d2r)/65535.0)); // End of D2R
    float rrPos=(float(maxRr-(float)rr)/float(maxRr)); // end of RR

    // shrink all the x positions horizontally
    arPos/=2.0;
    drPos/=2.0;
    d2rPos/=2.0;
    rrPos/=1.0;

    ImVec2 pos1=ImLerp(rect.Min,rect.Max,ImVec2(0.0,1.0)); // the bottom corner
    ImVec2 pos2=ImLerp(rect.Min,rect.Max,ImVec2(arPos,((float)tl/maxTl))); // peak of AR, start of DR
    ImVec2 pos3=ImLerp(rect.Min,rect.Max,ImVec2(drPos,(float)(((float)tl/maxTl)+((float)sl/65535.0)-(((float)tl/maxTl)*((float)sl/65535.0))))); // end of DR, start of D2R
    ImVec2 pos4=ImLerp(rect.Min,rect.Max,ImVec2(d2rPos,1.0)); // end of D2R
    ImVec2 posRStart=ImLerp(rect.Min,rect.Max,ImVec2(0.0,((float)tl/maxTl))); // release start
    ImVec2 posREnd=ImLerp(rect.Min,rect.Max,ImVec2(rrPos,1.0));// release end
    ImVec2 posSLineHEnd=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(float)(((float)tl/maxTl)+((float)sl/65535.0)-(((float)tl/maxTl)*((float)sl/65535.0))))); // sustain horizontal line end
    ImVec2 posSLineVEnd=ImLerp(rect.Min,rect.Max,ImVec2(drPos,1.0)); // sustain vertical line end
    ImVec2 posDecayRate0Pt=ImLerp(rect.Min,rect.Max,ImVec2(1.0,((float)tl/maxTl))); // Height of the peak of AR, forever
    ImVec2 posDecay2Rate0Pt=ImLerp(rect.Min,rect.Max,ImVec2(1.0,(float)(((float)tl/maxTl)+((float)sl/65535.0)-(((float)tl/maxTl)*((float)sl/65535.0))))); // Height of the peak of SR, forever

    // dl->Flags=ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedLinesUseTex;
    if ((float)ar==0.0) { // if AR = 0, the envelope never starts
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      addAALine(dl,pos1,pos4,color); // draw line on ground
    } else if ((float)dr==0.0 && (float)sl!=0.0) { // if DR = 0 and SL is not 0, then the envelope stays at max volume forever
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      // addAALine(dl,pos3,posSLineHEnd,colorS); // draw horiz line through sustain level
      // addAALine(dl,pos3,posSLineVEnd,colorS); // draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); // A
      addAALine(dl,pos2,posDecayRate0Pt,color); // Line from A to end of graph
    } else if ((float)d2r==0.0 || ((instType==DIV_INS_OPL || instType==DIV_INS_SNES || instType==DIV_INS_ESFM || instType==DIV_INS_CSE1) && sus==1.0) || (instType==DIV_INS_OPLL && egt!=0.0)) { // envelope stays at the sustain level forever
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      addAALine(dl,pos3,posSLineHEnd,colorR); // draw horiz line through sustain level
      addAALine(dl,pos3,posSLineVEnd,colorR); // draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); // A
      addAALine(dl,pos2,pos3,color); // D
      addAALine(dl,pos3,posDecay2Rate0Pt,color); // Line from D to end of graph
    } else { // draw graph normally
      dl->AddTriangleFilled(posRStart,posREnd,pos1,colorS); // draw release as shaded triangle behind everything
      addAALine(dl,pos3,posSLineHEnd,colorR); // draw horiz line through sustain level
      addAALine(dl,pos3,posSLineVEnd,colorR); // draw vert. line through sustain level
      addAALine(dl,pos1,pos2,color); // A
      addAALine(dl,pos2,pos3,color); // D
      addAALine(dl,pos3,pos4,color); // D2
    }
    //dl->Flags^=ImDrawListFlags_AntiAliasedLines|ImDrawListFlags_AntiAliasedLinesUseTex;
  }
}
