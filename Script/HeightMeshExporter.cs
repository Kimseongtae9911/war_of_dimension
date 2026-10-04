using UnityEngine;
using System.Collections.Generic;
using System.IO;

public class HeightMeshExporter : MonoBehaviour
{
    public string fileName = "NavObjectsHeightMesh.obj";

    private void Start()
    {
        GameObject[] navObjects = GameObject.FindGameObjectsWithTag("Nav");
        List<Vector3> vertices = new List<Vector3>();
        List<int> triangles = new List<int>();

        foreach (GameObject navObject in navObjects)
        {
            MeshFilter meshFilter = navObject.GetComponent<MeshFilter>();
            if (meshFilter != null)
            {
                Mesh mesh = meshFilter.sharedMesh;
                if (mesh != null)
                {
                    int vertexOffset = vertices.Count;

                    Vector3[] objectVertices = mesh.vertices;
                    int[] objectTriangles = mesh.triangles;

                    for (int i = 0; i < objectVertices.Length; i++)
                    {
                        vertices.Add(navObject.transform.TransformPoint(objectVertices[i]));
                    }

                    for (int i = 0; i < objectTriangles.Length; i += 3)
                    {
                        triangles.Add(objectTriangles[i] + vertexOffset);
                        triangles.Add(objectTriangles[i + 1] + vertexOffset);
                        triangles.Add(objectTriangles[i + 2] + vertexOffset);
                    }
                }
            }
        }

        using (StreamWriter sw = new StreamWriter(fileName))
        {
            foreach (Vector3 vertex in vertices)
            {
                sw.WriteLine("v {0} {1} {2}", vertex.x, vertex.y, vertex.z);
            }

            for (int i = 0; i < triangles.Count; i += 3)
            {
                sw.WriteLine("f {0} {1} {2}", triangles[i] + 1, triangles[i + 1] + 1, triangles[i + 2] + 1);
            }
        }

        Debug.Log("Nav objects height mesh exported as " + fileName);
    }
}
