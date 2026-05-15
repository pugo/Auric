
GLuint compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar log[512];
        glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        BOOST_LOG_TRIVIAL(error) << "OpenGL shader compilation failed: " << log;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint create_screen_program()
{
    constexpr const char* vertex_shader = R"(
        #version 150
        in vec2 a_pos;
        in vec2 a_uv;
        out vec2 v_uv;
        void main()
        {
            v_uv = a_uv;
            gl_Position = vec4(a_pos, 0.0, 1.0);
        }
    )";

    constexpr const char* fragment_shader = R"(
        #version 150
        in vec2 v_uv;
        out vec4 out_color;
        uniform sampler2D u_texture;
        uniform int u_enable_scanlines;
        uniform int u_enable_vertical_lines;
        uniform int u_enable_vignette;
        uniform float u_vignette_strength;
        uniform float u_texture_height;

        float hash(vec2 p)
        {
            return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
        }

        void main()
        {
            vec2 uv = v_uv;

            // One screen pixel in UV space at current output resolution.
            vec2 texel = vec2(abs(dFdx(v_uv.x)), abs(dFdy(v_uv.y)));
            vec4 c0 = texture(u_texture, uv).bgra;
            vec4 c1 = texture(u_texture, clamp(uv + vec2(texel.x, 0.0), 0.0, 1.0)).bgra;
            vec4 c2 = texture(u_texture, clamp(uv - vec2(texel.x, 0.0), 0.0, 1.0)).bgra;
            vec4 c3 = texture(u_texture, clamp(uv + vec2(0.0, texel.y), 0.0, 1.0)).bgra;
            vec4 c4 = texture(u_texture, clamp(uv - vec2(0.0, texel.y), 0.0, 1.0)).bgra;
            vec4 color = c0 * 0.50 + (c1 + c2) * 0.20 + (c3 + c4) * 0.05;

            float scanline = 0.96 + 0.04 * sin(v_uv.y * u_texture_height * 3.14159265);
            float mask = 0.99 + 0.01 * sin(gl_FragCoord.x * 0.3);
            vec2 centered = abs(v_uv * 2.0 - 1.0);
            float edge = max(centered.x * 0.85, centered.y);
            float vignette = 1.0 - u_vignette_strength * pow(edge, 2.2);
            float noise = (hash(gl_FragCoord.xy) - 0.5) * 0.02;

            if (u_enable_scanlines != 0) {
                color.rgb *= scanline;
            }

            if (u_enable_vertical_lines != 0) {
                color.rgb *= mask;
            }

            if (u_enable_vignette != 0) {
                color.rgb *= vignette;
            }

            color.rgb += noise;
            out_color = vec4(clamp(color.rgb, 0.0, 1.0), 1.0);
        }
    )";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
    if (vs == 0) {
        return 0;
    }
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);
    if (fs == 0) {
        glDeleteShader(vs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar log[512];
        glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        BOOST_LOG_TRIVIAL(error) << "OpenGL program link failed: " << log;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}
