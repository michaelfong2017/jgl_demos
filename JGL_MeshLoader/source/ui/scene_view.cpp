#include "pch.h"
#include "scene_view.h"

#include "imgui.h"

namespace nui
{
  void SceneView::resize()
  {
    mFrameBuffer->create_buffers((int32_t)mSize.x, (int32_t) mSize.y);
  }

  void SceneView::render_elems(nelems::Mesh* mesh)
  {

    mLight->update(mShader.get());

    mFrameBuffer->bind();

    mesh->render();

    mFrameBuffer->unbind();
  }

  void SceneView::on_mouse_move(double x, double y, nelems::EInputButton button)
  {
    mCamera->on_mouse_move(x, y, button);
  }

  void SceneView::set_distance(float distance)
  {
    mCamera->set_distance(distance);
  }

  void SceneView::render()
  {
    ImGui::Begin("Scene");

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    mSize = { viewportPanelSize.x, viewportPanelSize.y };

    mCamera->set_aspect(mSize.x / mSize.y);
    mCamera->update(mShader.get());

    uint64_t textureID = mFrameBuffer->get_texture();
    ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ mSize.x, mSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

    ImGui::End();
  }
}