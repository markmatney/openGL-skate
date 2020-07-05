#version 120

attribute   vec4 vPosition;
attribute   vec3 vNormal;
attribute   vec2 vTexCoords;

// output values that will be interpretated per-fragment
varying  vec3 fN;
varying  vec3 fE;
varying  vec3 fL;


uniform mat4 ModelView;
uniform mat4 View;
uniform vec4 LightPosition;
uniform mat4 Projection;

void main()
{
    vec4 N = vec4(vNormal, 0.0f);
    fN =  (ModelView * N).xyz; //not correct, should be inverse(transpose(ModelView)); only for version 150 or higher.
    fE = -(ModelView * vPosition).xyz;
    fL =  (LightPosition).xyz;
    //fL = (View * LightPosition).xyz;

    if( LightPosition.w != 0.0 ) 
    {
        fL = LightPosition.xyz - vPosition.xyz;
    }

    gl_Position = Projection * ModelView * vPosition;
    gl_TexCoord[0].xy = vTexCoords;
}
//