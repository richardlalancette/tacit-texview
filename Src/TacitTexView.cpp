// TacitTexView.cpp
//
// A texture viewer for various formats.
//
// Copyright (c) 2017, 2019 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tVersion.h>
#include <System/tCommand.h>
#include <Image/tTexture.h>
#include <System/tFile.h>
#include "TacitTexView.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h> 
using namespace tStd;


tCommand::tOption PrintAllOutput("Print all output.", 'a', "all");


// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct TextureViewerLog
{
	ImGuiTextBuffer     Buf;
	ImGuiTextFilter     Filter;
	ImVector<int>       LineOffsets;        // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
	bool                ScrollToBottom;
	TextureViewerLog() : ScrollToBottom(true)
	{
		Clear();
	}

	void    Clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendfv(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size + 1);
		ScrollToBottom = true;
	}

	void    Draw(const char* title, bool* p_open = NULL)
	{
//		if (!ImGui::Begin(title, p_open))
//		{
//			ImGui::End();
//			return;
//		}
		if (ImGui::Button("Clear")) Clear();
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		if (Filter.IsActive())
		{
			for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				if (Filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}
		}
		else
		{
			// The simplest and easy way to display the entire buffer:
			//ImGui::TextUnformatted(buf, buf_end); 
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
			// Here we instead demonstrate using the clipper to only process lines that are within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
			// Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height, 
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
			// Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
			ImGuiListClipper clipper;
			clipper.Begin(LineOffsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					ImGui::TextUnformatted(line_start, line_end);
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		if (ScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;
		ImGui::EndChild();
		//ImGui::End();
	}
};


static bool gLogOpen = true;
static TextureViewerLog gLog;


void ShowTextureViewerLog()
{
	// For the demo: add a debug button before the normal log window contents
	// We take advantage of the fact that multiple calls to Begin()/End() are appending to the same window.
	ImGui::SetNextWindowSize(ImVec2(600, 50), ImGuiCond_FirstUseEver);
	ImGui::SetCursorPosX(500.0f);
	ImGui::SetCursorPosY(0.0f);
	gLog.Draw("Log", &gLogOpen);
}


static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


void PrintRedirectCallback(const char* text, int numChars)
{
	gLog.AddLog("%s", text);
}


tImage::tPicture gPicture;
GLuint tex = 0;
tList<tStringItem> gFoundFiles;
tStringItem* gCurrFile = nullptr;

void LoadCurrFile()
{
	if (!gCurrFile)
		return;

	tPrintf("Loading Image: %s\n", gCurrFile->ConstText());
	gPicture.Load(*gCurrFile);

	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	tPrintf("Width: %d Height: %d\n", gPicture.GetWidth(), gPicture.GetHeight());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gPicture.GetWidth(), gPicture.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, gPicture.GetPixelPointer());

	glBindTexture(GL_TEXTURE_2D, 0);
}

void FindTextureFiles()
{
	tString currentDir = tSystem::tGetCurrentDir();
	tString imagesDir = currentDir + "Textures/";

	tPrintf("Looking for image files in %s\n", imagesDir.ConstText());
	tSystem::tFindFilesInDir(gFoundFiles, imagesDir, "*.jpg");
	tSystem::tFindFilesInDir(gFoundFiles, imagesDir, "*.gif");
	tSystem::tFindFilesInDir(gFoundFiles, imagesDir, "*.tga");
	tSystem::tFindFilesInDir(gFoundFiles, imagesDir, "*.png");
	tSystem::tFindFilesInDir(gFoundFiles, imagesDir, "*.tiff");
	gCurrFile = gFoundFiles.First();
	glGenTextures(1, &tex);
}

void DoFrame(GLFWwindow* window, bool dopoll = true)
{
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	if (dopoll)
		glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();		
	ImGui_ImplGlfw_NewFrame();

	int dispw, disph;
	glfwGetFramebufferSize(window, &dispw, &disph);
	float dispaspect = float(dispw)/float(disph);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, dispw, 0, disph, -1, 1);
	glMatrixMode(GL_MODELVIEW);

	//clear and draw quad with texture (could be in display callback)
	if (gPicture.IsValid())
	{
		ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindTexture(GL_TEXTURE_2D, tex);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);

		int w = gPicture.GetWidth();
		int h = gPicture.GetHeight();
		float picaspect = float(w)/float(h);

		float drawh = 0.0f;
		float draww = 0.0f;
		float hmargin = 0.0f;
		float vmargin = 0.0f;
		if (dispaspect > picaspect)
		{
			drawh = float(disph);
			draww = picaspect * drawh;
			hmargin = (dispw - draww) * 0.5f;
			vmargin = 0.0f;
		}
		else
		{
			draww = float(dispw);
			drawh = draww / picaspect;
			vmargin = (disph - drawh) * 0.5f;
			hmargin = 0.0f;
		}

		glTexCoord2i(0, 0); glVertex2f(hmargin, vmargin);
		glTexCoord2i(0, 1); glVertex2f(hmargin, vmargin+drawh);
		glTexCoord2i(1, 1); glVertex2f(hmargin+draww, vmargin+drawh);
		glTexCoord2i(1, 0); glVertex2f(hmargin+draww, vmargin);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFlush(); //don't need this with GLUT_DOUBLE and glutSwapBuffers
	}

    ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	bool show_demo_window = false;
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::BeginMainMenuBar();
	{
		if (ImGui::Button("Prev"))
		{
			if (gCurrFile && gCurrFile->Prev())
			{
				gCurrFile = gCurrFile->Prev();
				LoadCurrFile();
			}
		}
		if (ImGui::Button("Next"))
		{
			if (gCurrFile && gCurrFile->Next())
			{
				gCurrFile = gCurrFile->Next();
				LoadCurrFile();
			}
		}

		static ImVec4 colour;
		colour.x = 1.0f;	colour.y = 0.0f;	colour.z = 0.0f;	colour.w = 1.0f;
		ImGui::ColorButton("Colour", colour);
	}

	ImGui::EndMainMenuBar();
	ShowTextureViewerLog();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, dispw, disph);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}

void Windowrefreshfun(GLFWwindow* window)
{
	DoFrame(window, false);
}

int main(int, char**)
{
	tSystem::tSetStdoutRedirectCallback(PrintRedirectCallback);	

	tPrintf("Tacit Texture Viewer\n");
	tPrintf("Tacent Version %d.%d.%d\n", tVersion::Major, tVersion::Minor, tVersion::Revision);
	tPrintf("Dear IMGUI Version %s (%d)\n", IMGUI_VERSION, IMGUI_VERSION_NUM);

	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Tacent Texture Viewer", NULL, NULL);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync
	glfwSetWindowRefreshCallback(window, Windowrefreshfun);

    // Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

	io.Fonts->AddFontFromFileTTF("Data/Roboto-Medium.ttf", 14.0f);

	bool show_demo_window = true;
	bool show_another_window = false;

	FindTextureFiles();
	LoadCurrFile();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		DoFrame(window);
	}

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}