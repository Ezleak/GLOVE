#version 100

#ifdef GL_ES
  precision mediump float;
#endif

#define BLUR_SIZE  2

uniform sampler2D uniform_texture;
uniform vec2      uniform_resolution_div;

varying vec2      v_texCoord_out;

vec4 BoxBlurColor(vec2 coords)
{
    vec4 sum     = texture2D(uniform_texture, coords);

    int  samples = 1;
    for (int x = -BLUR_SIZE; x <= BLUR_SIZE; x++)
        for (int y = -BLUR_SIZE; y <= BLUR_SIZE; y++)
        {
            samples++;
            vec2 c = coords + vec2 ( float(x)*uniform_resolution_div.x, float(y)*uniform_resolution_div.y);
            sum += texture2D(uniform_texture, c);
        }
    return vec4(sum.rgb/vec3(samples), 1.0);
}

void main()
{
    gl_FragColor = BoxBlurColor(v_texCoord_out);
}
