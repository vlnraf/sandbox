#version 330

in vec4 OutColor;
in vec2 TexCoord;
flat in int TexIndex;

//layout(binding = 0) uniform sampler2D sprite[16];
//uniform sampler2D sprite[16];

out vec4 FragColor;

void main()
{
    vec2 uv = TexCoord;
    vec2 center = vec2(0.5, 0.5);
    vec2 centered = uv - center;
    float dist = distance(uv, center);
    float radius = 0.5;

    // Discard pixels outside the circle
    if(dist > radius){
        discard;
    }

    FragColor = OutColor;
    //if (FragColor.a <= 0.1) {
    //    //NOTE: was it necessary for the ysorting???
    //    discard; // Discards the fragment if alpha is less than or equal to 0.9
    //}
}