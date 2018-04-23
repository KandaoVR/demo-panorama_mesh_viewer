#pragma once
#define STRINGIFY(A) #A

static const char *show_texture_vs = STRINGIFY(
	\#version 330 core\n
	layout(location = 0) in vec3 aPos;
	layout(location = 1) in vec2 aTexCoord;

	out vec2 TexCoord;

	void main()
	{
		gl_Position = vec4(aPos, 1.0);
		TexCoord = vec2(aTexCoord.x, aTexCoord.y);
	}
);

static const char *show_equi_vs = STRINGIFY(
	\#version 330 core\n
	layout(location = 0) in vec3 aPos;
	layout(location = 1) in vec2 aTexCoord;

	out vec2 TexCoord;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(aPos, 1.0);
		TexCoord = vec2(aTexCoord.x, aTexCoord.y);
	}
);

static const char *show_texture_fs = STRINGIFY(
	\#version 330 core\n
	in vec2 TexCoord;
	out vec4 color;										

	uniform sampler2D texture0;							

	void main()											
	{														
		color = texture(texture0, TexCoord);
	}
);

#undef STRINGIFY
