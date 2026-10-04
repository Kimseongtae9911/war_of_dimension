#include "Common.hlsl"

//Skill Type
#define PARTICLE_TYPE_FOG		0 
#define PARTICLE_TYPE_DROPARROW		1
#define PARTICLE_TYPE_XYEMITTER		2
#define PARTICLE_TYPE_XZEMITTER		3
#define PARTICLE_TYPE_TAILSTAR		4
#define PARTICLE_TYPE_TORNADO		5
#define PARTICLE_TYPE_INSIDEMOVE	6
#define PARTICLE_TYPE_SPINSCENTER	7
#define PARTICLE_TYPE_FIRE			8
#define PARTICLE_TYPE_BUFF			9
#define PARTICLE_TYPE_PHOENIX		10
#define PARTICLE_TYPE_STORMARROW	11
#define PARTICLE_TYPE_FIGHTATTACK	12
#define PARTICLE_TYPE_BALL			13
#define PARTICLE_TYPE_BASH			14
#define PARTICLE_TYPE_AURABLADE		15
#define PARTICLE_TYPE_BILLBOARD		16
#define PARTICLE_TYPE_MAGICBALL		17
#define PARTICLE_TYPE_RAY		    18
#define PARTICLE_TYPE_SPHERE	    19
#define PARTICLE_TYPE_BIGBANG	    20
#define PARTICLE_TYPE_BILLBOARDFRONT 21
#define PARTICLE_TYPE_ROUND         22
#define PARTICLE_TYPE_JUMP          23
#define PARTICLE_TYPE_COIN          24
#define PARTICLE_TYPE_CRUSH         25
#define PARTICLE_TYPE_FENCE         26



//Emitter Type
#define PARTICLE_TYPE_FLAREBASIC		100 
#define PARTICLE_TYPE_FLAREDROPARROW	101 
#define PARTICLE_TYPE_FLARETAILSTAR		104 
#define PARTICLE_TYPE_FLARETORNADO		105 
#define PARTICLE_TYPE_FLARESPINCENTER	107 
#define PARTICLE_TYPE_FLAREFIRE			108 
#define PARTICLE_TYPE_FLARESTORMARROW	111
#define PARTICLE_TYPE_FLAREJUMP     	123
#define PARTICLE_TYPE_FLARECOIN     	125
#define PARTICLE_TYPE_FLAREBIGBANG    	120






Texture2D<float4> gtxtParticleTexture : register(t27);
//Texture1D<float4> gtxtRandom : register(t2);
Buffer<float4> gRandomBuffer : register(t28);
Buffer<float4> gRandomSphereBuffer : register(t29);

static const float gfSecondsPerFirework = 1.0f;
static const int gnFlareParticlesToEmit = 60;
static const float3 gf3Gravity = float3(0.0f, 0.0f, 0.0f);
static const float3 gf3TornadoGravity = float3(0.0f, 4.9f, 0.0f);
static const float3 gf3RainGravity = float3(0.0f, -9.8f, 0.0f);
static const int gnMaxFlareType2Particles = 15;


struct VS_PARTICLE_INPUT
{
    float3 position : POSITION;
    float3 velocity : VELOCITY;
    float lifetime : LIFETIME;
    uint type : PARTICLETYPE;
};

VS_PARTICLE_INPUT VSParticleStreamOutput(VS_PARTICLE_INPUT input)
{
    return (input);
}

float3 GetCameraForwardVector() //player forward vector
{
    matrix inverseView = gmtxInverseView;
    return normalize(inverseView[2].xyz);
}

float4 RandomDirection(float fOffset)
{
    int u = uint(gElapsedTime + fOffset + frac(gCurrentTime) * 1000.0f) % 1024;
    return (normalize(gRandomBuffer.Load(u)));
}

float4 RandomDirectionOnSphere(float fOffset)
{
    int u = uint(gElapsedTime + fOffset + frac(gCurrentTime) * 1000.0f) % 256;
    return (normalize(gRandomSphereBuffer.Load(u)));
}

void OutputParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    input.position += input.velocity * gElapsedTime;
    input.velocity += gf3Gravity * gElapsedTime;
    input.lifetime -= gElapsedTime;

    output.Append(input);
}

void OutputStarParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    input.position += input.velocity * gElapsedTime;
    input.velocity += gf3Gravity * gElapsedTime;
    input.lifetime -= gElapsedTime;

    output.Append(input);
}

void OutputArrowParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
        {
            input.position += input.velocity * gElapsedTime;
            input.velocity += gf3Gravity * gElapsedTime;
        }
        input.lifetime -= gElapsedTime;

        output.Append(input);
    }
}

void OutputCoinParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
        {
            input.position += input.velocity * gElapsedTime;
            input.velocity += gf3RainGravity * gElapsedTime;
        }
        input.lifetime -= gElapsedTime;

        output.Append(input);
    }
}



void OutputTornadoParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        input.position += input.velocity * gElapsedTime;
        input.lifetime -= gElapsedTime;

        float age = gfLifeTime - input.lifetime;
        float rotationAngle = 0.5f;


        float3x3 rotationMatrix = float3x3(
        cos(rotationAngle), 0, -sin(rotationAngle),
        0, 1, 0,
        sin(rotationAngle), 0, cos(rotationAngle)
    );
	
        input.velocity = mul(input.velocity, rotationMatrix);
	
        output.Append(input);
    }
}

void OutputJumpParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
        {
            input.position += input.velocity * gElapsedTime;
            input.velocity += gf3TornadoGravity * gElapsedTime * 2.f;
        }
    
        input.lifetime -= gElapsedTime;

        output.Append(input);
    }
}

void OutputSpinCenterParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
        {
            input.position += input.velocity * gElapsedTime;
            input.velocity += gf3Gravity * gElapsedTime;
    

            float rotationAngle = gElapsedTime;


            float3x3 rotationMatrix = float3x3(
        cos(rotationAngle), 0, -sin(rotationAngle),
        0, 1, 0,
        sin(rotationAngle), 0, cos(rotationAngle)
    );
            input.velocity = mul(input.velocity, rotationMatrix);
        }

	
        input.lifetime -= gElapsedTime;
        output.Append(input);
    }
}

void OutputBingbangParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
        {
            input.position += input.velocity * gElapsedTime;
            input.velocity += gf3Gravity * gElapsedTime;
    

            float rotationAngle = gElapsedTime * 1.5f;


            float3x3 rotationMatrix = float3x3(
        cos(rotationAngle), 0, -sin(rotationAngle),
        0, 1, 0,
        sin(rotationAngle), 0, cos(rotationAngle)
    );
            input.velocity = mul(input.velocity, rotationMatrix);
        }

	
        input.lifetime -= gElapsedTime;
        output.Append(input);
    }
}


void OutputFireParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
        {
            input.position += input.velocity * gElapsedTime;
            input.velocity += gf3Gravity * gElapsedTime;
        }
    
        input.lifetime -= gElapsedTime;

        output.Append(input);
    }
}


void OutputStormArrowParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
            input.position += input.velocity * gElapsedTime;
        input.velocity += normalize(gvForwardVector) * gElapsedTime;
        input.lifetime -= gElapsedTime;

        output.Append(input);
    }
   
}

void OutputSpriteParticleToStream(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        if (input.lifetime < gfLifeTime)
            input.position += input.velocity * gElapsedTime;
        input.velocity += gf3Gravity * gElapsedTime;
        input.lifetime -= gElapsedTime;

        output.Append(input);
    }
   
}

void OutputEmberParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime > 0.0f)
    {
        OutputParticleToStream(input, output);
    }
}

void XZParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;
        float4 f4Random = float4(0.0f, 0.0f, 0.0f, 0.0f);

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.position = input.position + (input.velocity * gElapsedTime * 2.0f);
        particle.lifetime = gfLifeTime;

        for (int i = 0; i < giParticleNum; i++)
        {
            f4Random = RandomDirection(input.type + i);
            float3 RandomDirect = float3(f4Random.x, 0, f4Random.z) * 18.0f;
            particle.velocity = input.velocity + (RandomDirect);

            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void XYParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;
        float4 f4Random = float4(0.0f, 0.0f, 0.0f, 0.0f);

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.position = input.position + (input.velocity * gElapsedTime * 2.0f);
        particle.lifetime = gfLifeTime;

        for (int i = 0; i < giParticleNum; i++)
        {
            f4Random = RandomDirection(input.type + i);
            
            float3 cameraForward = normalize(gvForwardVector);
            float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
            float3 up = cross(cameraForward, right);
            float3 RandomDirect = (f4Random.x * right + f4Random.z * up) * 18.f;
            particle.velocity = input.velocity + (RandomDirect);

            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void StarParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;
        float4 f4Random = float4(0.0f, 0.0f, 0.0f, 0.0f);

        particle.type = PARTICLE_TYPE_FLAREBASIC;

        for (int i = 0; i < giParticleNum; i++)
        {
            f4Random = RandomDirection(input.type + i);
            
            float3 cameraForward = normalize(gvForwardVector);
            float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
            float3 up = cross(cameraForward, right);
            float3 RandomDirect = cameraForward;
            particle.velocity = input.velocity;
            particle.lifetime = gfLifeTime + (giParticleNum - i) * 0.05f;
            particle.position = input.position - RandomDirect * gElapsedTime * i * 3;
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void TornadoParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;
        float4 f4Random = float4(0.0f, 0.0f, 0.0f, 0.0f);

        particle.type = PARTICLE_TYPE_FLARETORNADO;
        particle.lifetime = gfLifeTime;

        float3 cameraForward = normalize(gvForwardVector);
        float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
        float3 up = float3(0, 1, 0);
        float radius = 0.7f;
        for (int i = 0; i < giParticleNum; i++)
        {
            float angle = i * (2 * 3.141592 / giParticleNum * 3);
			
            f4Random = RandomDirectionOnSphere(input.type + i);
			
            float x = input.position.x + radius * cos(angle) * cameraForward.x + radius * sin(angle) * right.x;
            float z = input.position.z + radius * cos(angle) * cameraForward.z + radius * sin(angle) * right.z;
            if (i % 3 == 0)
                particle.position = input.position + x * cameraForward + z * right;
            else if (i % 3 == 1)
                particle.position = input.position + x * cameraForward + z * right + float3(0, 0.2f, 0);
            else if (i % 3 == 2)
                particle.position = input.position + x * cameraForward + z * right + float3(0, 0.4f, 0);
            float3 RandomDirect = float3(clamp(f4Random.x, -3, 3), 0.5f, clamp(f4Random.z, -3, 3));
            //float3 RandomDirect = float3(0, 0.5f, 0);
            particle.velocity = input.velocity + (RandomDirect);

            output.Append(particle);
        }
    }
    else
    {
        OutputTornadoParticleToStream(input, output);
    }
}

void FenceParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREFIRE;
       
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            float3 cameraForward = normalize(gvForwardVector);
            float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
            if (i % 7 == 0)
                particle.position = input.position - right * 3.f;
            else if (i % 7 == 1)
                particle.position = input.position - right * 2.f;
            else if (i % 7 == 2)
                particle.position = input.position - right;
            else if (i % 7 == 3)
                particle.position = input.position;
            else if (i % 7 == 4)
                particle.position = input.position + right;
            else if (i % 7 == 5)
                particle.position = input.position + right * 2.f;
            else if (i % 7 == 6)
                particle.position = input.position + right * 3.f;
            float3 Upvelocity = float3(0, 1.0f, 0);
            particle.velocity = input.velocity + Upvelocity;
            particle.lifetime = gfLifeTime + saturate(f4Random.x) * 10.f;
            output.Append(particle);
        }
    }
    else
    {
        OutputFireParticleToStream(input, output);
    }
}

void ArrowParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREDROPARROW;
        
        
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = float3(0.0f, -9.8f, 0.0f);
            particle.position = input.position + (f4Random.xyz * 2.0f) + float3(0.0f, 20.8f, 0.0f);
            particle.lifetime = gfLifeTime + saturate(f4Random.y) * 5.f;
			
            output.Append(particle);
        }
    }
    else
    {
        OutputArrowParticleToStream(input, output);
    }
}


void FogMovementParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.position = input.position + (input.velocity * gElapsedTime * 0.25f);
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity + (f4Random.xyz * 1.0f);

            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void InsideParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        //particle.position = input.position + (input.velocity * gElapsedTime * 0.25f);
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.position = input.position + float3(f4Random.x, f4Random.y, f4Random.z) * 5.f;
            float3 centerToPosition = float3(0, 0, 0) - particle.position;
            particle.velocity = input.velocity + centerToPosition * gElapsedTime * 15.f;
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void SpinCenterParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;
        float4 f4Random = float4(0.0f, 0.0f, 0.0f, 0.0f);

        particle.type = PARTICLE_TYPE_FLARESPINCENTER;

        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            float3 cameraForward = normalize(gvForwardVector);
            float3 RandomDirect = float3(0, 0.5f, 0);
            float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
            if (i % 4 == 0)
            {
                particle.position = input.position + right * 1.f;
                particle.velocity = (input.velocity + RandomDirect) * 10 + (-cameraForward) * 1.f;
            }
            else if (i % 4 == 1)
            {
                particle.position = input.position + cameraForward * 1.f;
                particle.velocity = (input.velocity + RandomDirect) * 10 + (right) * 1.f;
            }
            else if (i % 4 == 2)
            {
                particle.position = input.position + right * -1.f;
                particle.velocity = (input.velocity + RandomDirect) * 10 + (cameraForward) * 1.f;
            }
            else if (i % 4 == 3)
            {
                particle.position = input.position + cameraForward * -1.f;
                particle.velocity = (input.velocity + RandomDirect) * 10 + (-right) * 1.f;
            }
			
            particle.lifetime = gfLifeTime + floor(i / 4) * 0.1f;
            output.Append(particle);
        }
    }
    else
    {
        OutputSpinCenterParticleToStream(input, output);
    }
}

void FireParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREFIRE;
       
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            float3 cameraForward = normalize(gvForwardVector);
            float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
            if (i % 3 == 0)
                particle.position = input.position + right / 3;
            else if (i % 3 == 1)
                particle.position = input.position;
            else if (i % 3 == 2)
                particle.position = input.position - right / 3;
            float3 Upvelocity = float3(0, 0.6f, 0);
            particle.velocity = input.velocity + Upvelocity;
            particle.lifetime = gfLifeTime + saturate(f4Random.x) * 10.f;
            output.Append(particle);
        }
    }
    else
    {
        OutputFireParticleToStream(input, output);
    }
}

void BUFFParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity;
            particle.position = input.position + float3(0, f4Random.y, 0);

            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void PhoenixParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLARESPINCENTER;
        particle.position = input.position + (input.velocity * gElapsedTime * 2.0f);
        float3 cameraForward = normalize(gvForwardVector);
        float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
        float3 up = float3(0, 1, 0);
        float radius = 0.7f;
		
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            float3 vortexDirection = f4Random.x * -cameraForward + f4Random.y * right + f4Random.z * up;
            particle.velocity = input.velocity + vortexDirection;
            particle.lifetime = gfLifeTime + saturate(f4Random.x) * 10.f;

            output.Append(particle);
        }
    }
    else
    {
        OutputSpinCenterParticleToStream(input, output);
    }
}

void StormArrowParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;
        float4 f4Random = float4(0.0f, 0.0f, 0.0f, 0.0f);

        particle.type = PARTICLE_TYPE_FLARESTORMARROW;
		
        float3 cameraForward = normalize(gvForwardVector);
        float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
        float3 up = cross(cameraForward, right);
        float randX[5];
        float randY[5];
        for (int a = 0; a < 5; a++)
        {
            randX[a] = RandomDirectionOnSphere(input.type + a).x;
            randY[a] = RandomDirectionOnSphere(input.type + a).y;
        }
		
        for (int i = 0; i < giParticleNum; i++)
        {
            float3 RandomDirect = cameraForward;
            particle.velocity = cameraForward * 15.f;
            particle.lifetime = gfLifeTime + (i % 5) * 0.4f; //+ (giParticleNum - i / 5) * 0.05f;
            particle.position = input.position - cameraForward * gElapsedTime * 5 * i / 5 + right * randX[i % 5] + up * randY[i % 5];
            output.Append(particle);
        }
    }
    else
    {
        OutputStormArrowParticleToStream(input, output);
    }
}

void FightAttackParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.lifetime = gfLifeTime;
        float3 cameraForward = normalize(gvForwardVector);
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity;
            particle.position = input.position + float3(f4Random.x, f4Random.y, f4Random.z) + cameraForward;

            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void BALLParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity;
            particle.position = input.position + float3(f4Random.x, f4Random.y, f4Random.z);
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void BashParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREFIRE;
        particle.lifetime = gfLifeTime;
        particle.position = input.position + float3(0, 0.5, 0);
        
        float3 cameraForward = normalize(gvForwardVector);
        float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
       
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            

            float3 radiusDirection = normalize(input.position - cameraForward);


            float3 velocityDirection = normalize(float3(f4Random.x, f4Random.y, f4Random.z) + radiusDirection);


            float velocityMagnitude = 1.0f;
            particle.velocity = velocityDirection * velocityMagnitude * 3.f;

           
            output.Append(particle);
        }
    }
    else
    {
        OutputFireParticleToStream(input, output);
    }
}

void AuraBladeParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        float3 cameraForward = normalize(gvForwardVector);
        float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
        float3 up = float3(0, 1, 0);
        particle.lifetime = gfLifeTime;
		
        float radius = 0.5f;
        int numParticles = giParticleNum;
        float angleStep = 3.141592f / (numParticles - 1);
		
        for (int i = 0; i < giParticleNum; i++)
        {
            float angle = -3.141592f / 2.f + angleStep * i;
            float4 f4Random = RandomDirection(input.type + i);
            if (abs(f4Random.x) > 0.5)
                particle.velocity = input.velocity + float3(0.f, 0.3f, 0.f);
            else
                particle.velocity = input.velocity;
            
            particle.position = input.position - cameraForward * radius + cameraForward * radius * cos(angle) + up * radius * sin(angle);
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void RoundParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        float3 cameraForward = normalize(gvForwardVector);
        float3 right = normalize(cross(float3(0, 1, 0), cameraForward));
        float3 up = float3(0, 1, 0);
        particle.lifetime = gfLifeTime;
		
        float radius = 10.f;
        float angleStep = 2.f * 3.141592f * 9.f / float(giParticleNum);
		
        for (int i = 0; i < giParticleNum; i++)
        {
            float angle = angleStep * i;
			
            particle.velocity = input.velocity;
            particle.position = input.position + cameraForward * radius * cos(angle) + right * radius * sin(angle);
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void BillBoardBladeParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.lifetime = gfLifeTime;

        float3 up = float3(0, 1, 0);
        for (int i = 0; i < giParticleNum; i++)
        {
            particle.velocity = input.velocity;
            particle.position = input.position + up / 2;
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void BillBoardFrontBladeParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.lifetime = gfLifeTime;

        float3 cameraForward = normalize(gvForwardVector);
        float3 up = float3(0, 1, 0);
        for (int i = 0; i < giParticleNum; i++)
        {
            particle.velocity = input.velocity;
            particle.position = input.position + up / 2 + cameraForward / 2;
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void JumpParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREJUMP;
        

        float3 up = float3(0, 1, 0);
        for (int i = 0; i < giParticleNum; i++)
        {
            particle.velocity = input.velocity + float3(0, 1.5, 0);
            particle.position = input.position;
            particle.lifetime = gfLifeTime + i;
            output.Append(particle);
        }
    }
    else
    {
        OutputJumpParticleToStream(input, output);
    }
}

void MagicBallParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity;
            particle.position = input.position + float3(f4Random.x / 4, f4Random.y / 4, f4Random.z / 4);
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void RayParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        
        particle.lifetime = gfLifeTime;
        float3 cameraForward = normalize(gvForwardVector);
		
        for (int i = 0; i < giParticleNum; i++)
        {
            particle.velocity = input.velocity;
            particle.position = input.position + cameraForward * gfParticleSize * i;
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}


void SphereParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        
        particle.lifetime = gfLifeTime;
        float3 cameraForward = normalize(gvForwardVector);
		
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity;
            particle.position = input.position + float3(f4Random.x, f4Random.y, f4Random.z);
			
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

void BigbangParticle(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBIGBANG;
        
        particle.lifetime = gfLifeTime;
		

        float3 up = float3(0, 1, 0);
        
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            f4Random *= 12;
            
            particle.position = input.position + float3(f4Random.x, f4Random.y, f4Random.z);
             // 구의 반지름 벡터
            float3 radiusDirection = normalize(particle.position - input.position);

            // 구의 표면으로 투영한 지점
            float3 projectedPosition = normalize(particle.position) * 12.0f;

            // 파티클의 위치와 구의 표면으로 투영한 지점을 연결하는 벡터 (구의 xz평면에 대해서 접하는 벡터)
            float3 tangentVector = normalize(projectedPosition - particle.position);

            // 구한 접선 벡터를 파티클의 속도로 설정
            float velocityMagnitude = 10.0f; // 원하는 속도 크기를 조절
           
            particle.velocity = tangentVector * velocityMagnitude;
			
            particle.velocity.y = 0;
            
            output.Append(particle);
        }
    }
    else
    {
        OutputBingbangParticleToStream(input, output);
    }
}

void CoinParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLARECOIN;
        particle.position = input.position + (input.velocity * gElapsedTime);
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.velocity = input.velocity + (f4Random.xyz * 5.0f);

            output.Append(particle);
        }
    }
    else
    {
        OutputCoinParticleToStream(input, output);
    }
}


void CrushParticles(VS_PARTICLE_INPUT input, inout PointStream<VS_PARTICLE_INPUT> output)
{
    if (input.lifetime <= 0.0f)
    {
        VS_PARTICLE_INPUT particle = input;

        particle.type = PARTICLE_TYPE_FLAREBASIC;
        particle.velocity = input.velocity;
        particle.lifetime = gfLifeTime;
        for (int i = 0; i < giParticleNum; i++)
        {
            float4 f4Random = RandomDirectionOnSphere(input.type + i);
            particle.position = input.position + float3(f4Random.x, abs(f4Random.y), f4Random.z) * 12.f;
            output.Append(particle);
        }
    }
    else
    {
        OutputParticleToStream(input, output);
    }
}

[maxvertexcount(128)]
void GSParticleStreamOutput(point VS_PARTICLE_INPUT input[1], inout PointStream<VS_PARTICLE_INPUT> output)
{
    VS_PARTICLE_INPUT particle = input[0];

    if (particle.type == PARTICLE_TYPE_FOG)
        FogMovementParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_DROPARROW)
        ArrowParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_XYEMITTER)
        XYParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_XZEMITTER)
        XZParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_TAILSTAR)
        StarParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_TORNADO)
        TornadoParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_INSIDEMOVE)
        InsideParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_SPINSCENTER)
        SpinCenterParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_FIRE)
        FireParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_BUFF)
        BUFFParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_PHOENIX)
        PhoenixParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_STORMARROW)
        StormArrowParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_FIGHTATTACK)
        FightAttackParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_BALL)
        BALLParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_BASH)
        BashParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_AURABLADE)
        AuraBladeParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_BILLBOARD)
        BillBoardBladeParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_MAGICBALL)
        MagicBallParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_RAY)
        RayParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_SPHERE)
        SphereParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_BIGBANG)
        BigbangParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_BILLBOARDFRONT)
        BillBoardFrontBladeParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_ROUND)
        RoundParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_JUMP)
        JumpParticle(particle, output);
    else if (particle.type == PARTICLE_TYPE_COIN)
        CoinParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_CRUSH)
        CrushParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_FENCE)
        FenceParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLAREBASIC) //EmitterType
        OutputEmberParticles(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLAREDROPARROW)
        OutputArrowParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLARETORNADO)
        OutputTornadoParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLARESPINCENTER)
        OutputSpinCenterParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLAREFIRE)
        OutputFireParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLARETAILSTAR)
        OutputStarParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLARESTORMARROW)
        OutputStormArrowParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLAREJUMP)
        OutputJumpParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLARECOIN)
        OutputCoinParticleToStream(particle, output);
    else if (particle.type == PARTICLE_TYPE_FLAREBIGBANG)
        OutputBingbangParticleToStream(particle, output);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

struct VS_PARTICLE_DRAW_OUTPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float size : SCALE;
    uint type : PARTICLETYPE;
};

struct GS_PARTICLE_DRAW_OUTPUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXTURE;
    uint type : PARTICLETYPE;
};

VS_PARTICLE_DRAW_OUTPUT VSParticleDraw(VS_PARTICLE_INPUT input)
{
    VS_PARTICLE_DRAW_OUTPUT output = (VS_PARTICLE_DRAW_OUTPUT) 0;
    output.position = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
    //output.position = input.position;
    output.size = 0.5f;
    output.type = input.type;

    if (input.type == PARTICLE_TYPE_FOG)
    {
        output.color = float4(0.1f, 0.0f, 1.0f, 1.0f);
    }
    else if (input.type == PARTICLE_TYPE_DROPARROW)
    {
        output.color = float4(1.0f, 0.1f, 0.1f, 1.0f);
    }
    else if (input.type == PARTICLE_TYPE_FLAREBASIC || input.type == PARTICLE_TYPE_FLARETORNADO || input.type == PARTICLE_TYPE_FLARESPINCENTER || input.type == PARTICLE_TYPE_FLAREFIRE || input.type == PARTICLE_TYPE_FLAREDROPARROW || input.type == PARTICLE_TYPE_FLARESTORMARROW || input.type == PARTICLE_TYPE_FLAREJUMP || input.type == PARTICLE_TYPE_FLARECOIN || input.type == PARTICLE_TYPE_FLAREBIGBANG) //EmitterType
    {
        output.color = gvColor;
        if (input.lifetime < 0.8f)
            output.color.a = input.lifetime;
    }

    return (output);
}

static float3 gf3Positions[4] = { float3(-1.0f, +1.0f, 0.5f), float3(+1.0f, +1.0f, 0.5f), float3(-1.0f, -1.0f, 0.5f), float3(+1.0f, -1.0f, 0.5f) };
static float3 gf3LeanPositions[4] = { float3(-1.0f, 0.5f, +1.0f), float3(+1.0f, 0.5f, +1.0f), float3(-1.0f, 0.5f, -1.0f), float3(+1.0f, 0.5f, -1.0f) };
static float2 gf2QuadUVs[4] = { float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f) };

[maxvertexcount(4)]
void GSParticleDraw(point VS_PARTICLE_DRAW_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_DRAW_OUTPUT> outputStream)
{
    GS_PARTICLE_DRAW_OUTPUT output = (GS_PARTICLE_DRAW_OUTPUT) 0;

    output.type = input[0].type;
    output.color = input[0].color;
    float2 UVs[4] = { float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f) };
    if (gTotalSpriteNum != 0)
    {
        int widthNum = gCurrentSpriteNum % gWidthSpriteNum;
        float width = 1.f / float(gWidthSpriteNum);
        int TotalHeight = gTotalSpriteNum / gWidthSpriteNum;
        int HeightNum = int(gCurrentSpriteNum / gWidthSpriteNum);
        float Height = 1.f / float(TotalHeight);
		
        UVs[0] = float2(width * widthNum, Height * HeightNum);
        UVs[1] = float2(width * (widthNum + 1), Height * HeightNum);
        UVs[2] = float2(width * widthNum, Height * (HeightNum + 1));
        UVs[3] = float2(width * (widthNum + 1), Height * (HeightNum + 1));
    }
	
    for (int i = 0; i < 4; i++)
    {
        float3 positionW = mul(gf3Positions[i] * gfParticleSize, (float3x3) gmtxInverseView) + input[0].position;
        if (giboolLean == 1)
            positionW = mul(gf3LeanPositions[i] * gfParticleSize, (float3x3) gmtxInverseView) + input[0].position;
        output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);
        output.uv = gf2QuadUVs[i];
        if (gTotalSpriteNum != 0)
        {
            output.uv = UVs[i];
        }

        outputStream.Append(output);
    }
    outputStream.RestartStrip();
}

float4 PSParticleDraw(GS_PARTICLE_DRAW_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtParticleTexture.Sample(gssWrap, input.uv);
    cColor *= input.color;
	
    return (cColor);
}
