#include "txpch.hpp"

#include <frameio/ImGui/Nodes.hpp>
#include <frameio/ImGui/Widgets.hpp>
#include <frameio/frameio.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>

// #include "LookupNodes.hpp"
#include "Nodes.hpp"

namespace Texturia {

class ViewportLayer : public Frameio::Layer {
public:
  ViewportLayer()
      : Layer("Texturia: Gui"),
        m_Camera(-1.6f, 1.6f, -0.9f, 0.9f),
        m_CameraMoveDirection(0.0f),
        m_BackgroundPosition(0.0f),
        m_BackgroundScale(1.5f),
        m_TrianglePosition(-0.2f),
        m_TriangleScale(1.0f)
  {
    Frameio::BufferLayout bufferLayout = {
      {Frameio::ShaderDataType::Float3,     "a_Position"},
      {Frameio::ShaderDataType::Float2, "a_TextureCoord"},
      {Frameio::ShaderDataType::Float4,        "a_Color"}
    };

    // TRIANGLE
    m_TriangleVertexArray = Frameio::VertexArray::Create();

    float triangleVertices[3 * 9] = {
      // clang-format off
      //    x,     y,    z,     u,     v,    r,    g,    b,    a
         0.0f,  0.5f, 0.0f,  0.5f,  1.0f, 0.1f, 0.6f, 1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  1.0f,  0.0f, 0.2f, 0.3f, 0.9f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f,  0.0f, 0.4f, 0.1f, 1.0f, 1.0f
      // clang-format on
    };

    uint32_t triangleIndices[3] = { 0, 1, 2 };

    Frameio::Ref<Frameio::VertexBuffer> triangleVertexBuffer;
    triangleVertexBuffer = Frameio::VertexBuffer::Create(sizeof(triangleVertices), triangleVertices);

    triangleVertexBuffer->SetLayout(bufferLayout);
    m_TriangleVertexArray->AddVertexBuffer(triangleVertexBuffer);

    Frameio::Ref<Frameio::IndexBuffer> triangleIndexBuffer;
    triangleIndexBuffer = Frameio::IndexBuffer::Create(sizeof(triangleIndices) / sizeof(uint32_t), triangleIndices);

    m_TriangleVertexArray->SetIndexBuffer(triangleIndexBuffer);
    // END TRIANGLE

    // SQUARE
    m_BackgroundVertexArray = Frameio::VertexArray::Create();

    float squareVertices[4 * 9] = {
      // clang-format off
      //    x,     y,    z,     u,     v,    r,    g,    b,    a
        -0.5f,  0.5f, 0.0f,  0.0f,  1.0f, 0.7f, 0.3f, 0.2f, 1.0f,
         0.5f,  0.5f, 0.0f,  1.0f,  1.0f, 0.9f, 0.3f, 0.3f, 1.0f,
         0.5f, -0.5f, 0.0f,  1.0f,  0.0f, 0.7f, 0.4f, 0.2f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f,  0.0f, 0.8f, 0.2f, 0.2f, 1.0f
      // clang-format on
    };

    uint32_t squareIndices[6] = { 0, 1, 2, 0, 3, 2 };

    Frameio::Ref<Frameio::VertexBuffer> squareVertexBuffer;
    squareVertexBuffer = Frameio::VertexBuffer::Create(sizeof(squareVertices), squareVertices);

    squareVertexBuffer->SetLayout(bufferLayout);
    m_BackgroundVertexArray->AddVertexBuffer(squareVertexBuffer);

    Frameio::Ref<Frameio::IndexBuffer> squareIndexBuffer;
    squareIndexBuffer = Frameio::IndexBuffer::Create(sizeof(squareIndices) / sizeof(uint32_t), squareIndices);

    m_BackgroundVertexArray->SetIndexBuffer(squareIndexBuffer);
    // END SQUARE

    std::string vertexSource =
        R"(
        #version 330 core

        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec2 a_TextureCoord;
        layout(location = 2) in vec4 a_Color;

        uniform mat4 u_ViewProjectionMatrix;
        uniform mat4 u_TransformMatrix;

        out vec3 v_Position;
        out vec2 v_TextureCoord;
        out vec4 v_Color;

        void main() {
          v_Position = a_Position;
          v_TextureCoord = a_TextureCoord;
          v_Color = a_Color;
          gl_Position = u_ViewProjectionMatrix * u_TransformMatrix * vec4(a_Position, 1.0);
        }
      )";

    std::string fragSrcTextureCoord =
        R"(
        #version 330 core

        layout(location = 0) out vec4 o_Color;

        in vec2 v_TextureCoord;

        void main() {
          o_Color = vec4(v_TextureCoord, 0.0, 1.0);
        }
      )";

    std::string fragSrcVertexColor =
        R"(
        #version 330 core

        layout(location = 0) out vec4 o_Color;

        in vec4 v_Color;

        void main() {
          o_Color = v_Color;
        }
      )";

    std::string fragSrcFlatColor =
        R"(
        #version 330 core

        layout(location = 0) out vec4 o_Color;

        uniform vec4 u_FlatColor;

        void main() {
          o_Color = u_FlatColor;
        }
      )";

    std::string fragSrcTexture =
        R"(
        #version 330 core

        layout(location = 0) out vec4 o_Color;

        in vec2 v_TextureCoord;

        uniform sampler2D u_Texture;

        void main() {
          o_Color = texture(u_Texture, v_TextureCoord);
        }
      )";

    m_DebugShader = Frameio::Shader::Create(vertexSource, fragSrcVertexColor);
    m_TextureShader = Frameio::Shader::Create(vertexSource, fragSrcTexture);

    m_GridTexture = Frameio::Texture2D::Create("assets/textures/Grid.png");
    m_GridWithDotTexture = Frameio::Texture2D::Create("assets/textures/GridWithDot.png");

    m_TextureShader->Bind();
    std::dynamic_pointer_cast<Frameio::OpenGLShader>(m_TextureShader)->UploadUniformInt("u_Texture", 2);
  }

  void OnUpdate(Frameio::RealDeltaTime realDeltaTime) override
  {
    m_Time += realDeltaTime;

    m_CameraMoveDirection = glm::rotateZ(
        glm::vec3(0.0f, m_CameraMoveSpeed * realDeltaTime, 0.0f),
        glm::pi<float>() -
            glm::orientedAngle(glm::normalize(glm::vec2(0.0f, 1.0f)),
                               glm::normalize(glm::vec2(
                                   // TODO Access window from layer
                                   Frameio::Input::GetMouseX() / 1280 * 2 - 1,
                                   Frameio::Input::GetMouseY() / 720 /* m_Window->GetHeight( )*/ * 2 - 1))));

    if (Frameio::Input::IsKeyDown(FR_KEY_W)) m_Camera.SetPosition(m_Camera.GetPosition() + m_CameraMoveDirection);

    if (Frameio::Input::IsKeyDown(FR_KEY_A))
      m_Camera.SetPosition(m_Camera.GetPosition() +
                           glm::vec3(-m_CameraMoveSpeed * realDeltaTime,
                                     0.0f,
                                     0.0f) /* glm::rotateZ(m_CameraMoveDirection, glm::half_pi<float>()) */);

    if (Frameio::Input::IsKeyDown(FR_KEY_S))
      m_Camera.SetPosition(m_Camera.GetPosition() + glm::rotateZ(m_CameraMoveDirection, glm::pi<float>()));

    if (Frameio::Input::IsKeyDown(FR_KEY_D))
      m_Camera.SetPosition(m_Camera.GetPosition() +
                           glm::vec3(m_CameraMoveSpeed * realDeltaTime,
                                     0.0f,
                                     0.0f) /* glm::rotateZ(m_CameraMoveDirection, -glm::half_pi<float>()) */);

    // m_Camera.SetRotation(glm::degrees(glm::orientedAngle(
    //     glm::vec2(0.0f, -1.0f),
    //     glm::normalize(glm::vec2(Input::GetMouseX() / 1280 * 2 - 1,
    //                              Input::GetMouseY() / 720 * 2 - 1)))));

    m_Camera.SetRotation(glm::sin(m_Time));

    Frameio::RenderCommand::SetClearColor({ 0.09f, 0.09f, 0.09f, 1.0f });
    Frameio::RenderCommand::Clear();

    Frameio::Renderer::BeginScene(m_Camera);

    // Background
    // std::dynamic_pointer_cast<Frameio::OpenGLShader>(m_DebugShader)
    //     ->UploadUniformFloat4("u_FlatColor", { 0.8f, 0.1f, 0.2f, 1.0f });
    Frameio::Renderer::Submit(m_BackgroundVertexArray, m_DebugShader, glm::scale(glm::vec3(1.6f * 2, 0.9f * 2, 1.0f)));

    // Square
    m_GridWithDotTexture->Bind(2);
    Frameio::Renderer::Submit(
        m_BackgroundVertexArray, m_TextureShader, glm::scale(glm::translate(m_BackgroundPosition), m_BackgroundScale));

    // Triangle
    m_GridTexture->Bind(2);
    // std::dynamic_pointer_cast<Frameio::OpenGLShader>(m_DebugShader)
    //     ->UploadUniformFloat4("u_FlatColor", { 0.1f, 0.2f, 0.8f, 1.0f });
    Frameio::Renderer::Submit(
        m_TriangleVertexArray, m_TextureShader, glm::scale(glm::translate(m_TrianglePosition), m_TriangleScale));

    Frameio::Renderer::EndScene();
  }

  void OnImGuiRender() override
  {
    ImGui::Begin("Renderer Debug");
    ImGui::PushID("Triangle");
    ImGui::Text("Triangle");
    ImGui::DragFloat3("Position", m_TrianglePosition);
    // TODO Implement rotation of objects
    ImGui::DragFloat3("Rotation", m_TrianglePosition);
    ImGui::DragFloat3("Scale", m_TriangleScale, 1.0f);
    ImGui::PopID();
    ImGui::PushID("Background");
    ImGui::Text("Background");
    ImGui::DragFloat3("Position", m_BackgroundPosition);
    // TODO Implement rotation of objects
    ImGui::DragFloat3("Rotation", m_BackgroundPosition);
    ImGui::DragFloat3("Scale", m_BackgroundScale, 1.0f);
    ImGui::PopID();

    ImGui::End();
  }

private:
  float m_Time = 0.0f;

  Frameio::OrthographicCamera m_Camera;
  float m_CameraMoveSpeed = 1.5f;

  glm::vec3 m_CameraMoveDirection;
  Frameio::Ref<Frameio::Texture2D> m_GridTexture, m_GridWithDotTexture;
  Frameio::Ref<Frameio::Shader> m_DebugShader, m_TextureShader;
  Frameio::Ref<Frameio::VertexArray> m_TriangleVertexArray;
  glm::vec3 m_TrianglePosition;
  glm::vec3 m_TriangleScale;
  Frameio::Ref<Frameio::VertexArray> m_BackgroundVertexArray;
  glm::vec3 m_BackgroundPosition;
  glm::vec3 m_BackgroundScale;
};

// TODO find out how to use m_NodesTree in here
// TODO display more information about the Node by getting its data from m_NodesTree
//! The displayed ID might not be the same as the one saved in our Node object because we use uint64_t and not int
void MiniMapNodeHoverCallback(int nodeUUID, void* userData)
{
  ImGui::SetTooltip("Node UUID: %d", nodeUUID);
}

class GuiLayer : public Frameio::Layer {
public:
  GuiLayer() : Layer("Texturia: Gui")
  {
    m_NodesTree = std::make_shared<NodesTree>("Main Nodes Tree");
    m_NodesTree->AddNode(Node("Old Node 1", 2147483647));
    //! This UUID causes ImNodes to use -2147483648 because of integer overflow
    m_NodesTree->AddNode(Node("Old Node 2", 2147483648));

    m_NodesTree->AddNode(Node("New Node 1", Frameio::UUID(Frameio::Int32Range)));
    m_NodesTree->AddNode(Node("New Node 2", Frameio::UUID(Frameio::Int32Range)));
  }

  void OnImGuiRender() override
  {
    static bool showDemoWindow = false;
    static bool showMetricsWindow = true;
    static bool showNodesEditorWindow = false;

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("ImGui Demo")) showDemoWindow = showDemoWindow ? false : true;
        if (ImGui::MenuItem("Metrics")) showMetricsWindow = showMetricsWindow ? false : true;
        if (ImGui::MenuItem("Nodes Editor")) showMetricsWindow = showNodesEditorWindow ? false : true;
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Platform")) {
#ifdef TX_PLATFORM_WINDOWS
        ImGui::MenuItem("Windows");
#endif
#ifdef TX_PLATFORM_LINUX
        ImGui::MenuItem("Linux");
#endif
#ifdef TX_PLATFORM_MACOS
        ImGui::MenuItem("MacOS");
#endif
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);

    if (showMetricsWindow) ImGui::ShowMetricsWindow(&showMetricsWindow);

    // // Note that since many nodes can be selected at once, we first need to query the number of
    // // selected nodes before getting them.
    // static std::vector<int> selectedNodes;
    // const int numberSelectedNodes = ImNodes::NumSelectedNodes();
    // if (numberSelectedNodes > 0) {
    //   selectedNodes.resize(numberSelectedNodes);
    //   ImNodes::GetSelectedNodes(selectedNodes.data());
    // } else {
    //   selectedNodes.clear();
    // }

    if (showNodesEditorWindow) {
      static std::vector<std::pair<int, int>> links;

      ImGui::Begin("Nodes Editor");
      ImNodes::BeginNodeEditor();

      m_NodesTree->OnImGuiRender();

      ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomLeft, MiniMapNodeHoverCallback);

      for (int i = 0; i < links.size(); ++i) {
        const std::pair<int, int> link = links[i];
        ImNodes::Link(i, link.first, link.second);
      }

      ImNodes::EndNodeEditor();

      // TODO Integrate linking in own Nodes API with own UUIDs
      int start_attr, end_attr;
      if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) { links.push_back(std::make_pair(start_attr, end_attr)); }

      int linkID;
      if (ImNodes::IsLinkDestroyed(&linkID)) { links.erase(links.begin() + linkID); }

      ImGui::End();
    }
  }

  void OnEvent(Frameio::Event& event) override
  {
    if (event.GetEventType() == Frameio::EventType::KeyTyped) {
      Frameio::KeyTypedEvent& e = (Frameio::KeyTypedEvent&)event;
      FR_TRACE((char)e.GetKeyCode());
    }
  }

private:
  Frameio::Ref<NodesTree> m_NodesTree;
};

class TexturiaApp : public Frameio::App {
public:
  TexturiaApp()
  {
    PushLayer(new ViewportLayer());
    PushOverlay(new GuiLayer());
  }

  ~TexturiaApp() = default;
};

} // namespace Texturia

Frameio::App* Frameio::CreateApp()
{
  return new Texturia::TexturiaApp();
}
