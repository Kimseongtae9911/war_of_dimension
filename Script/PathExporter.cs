using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;
using System.IO;

public class PathFind : MonoBehaviour
{
    public string filePath;
    public Vector3 path1;
    public Vector3 path2;
    public Vector3 path3;
    public Vector3 path4;

    NavMeshAgent agent;
    ArrayList destinations;
    int count = 0;

    // Start is called before the first frame update
    void Start()
    {
        agent = GetComponent<NavMeshAgent>();

        destinations = new ArrayList();
        destinations.Add(path1);
        destinations.Add(path2);
        destinations.Add(path3);
        destinations.Add(path4);

        agent.SetDestination((Vector3)destinations[0]);

        NavMeshPath path = new NavMeshPath();
        agent.CalculatePath((Vector3)destinations[0], path);

        ExportCorners(path);
    }

    // Update is called once per frame
    void Update()
    {
        if (count >= 4)
        {

        }
        else
        {
            if (CheckDistance(agent.transform.position, ((Vector3)destinations[count])))
            {
                count++;
                agent.SetDestination((Vector3)destinations[count]);
                agent.isStopped = false;

                NavMeshPath path = new NavMeshPath();
                agent.CalculatePath((Vector3)destinations[count], path);

                ExportCorners(path);
            }
        }
    }

    bool CheckDistance(Vector3 v1, Vector3 v2)
    {
        if (Mathf.Abs(v1.x * v1.x - v2.x * v2.x) > 0.5f)
            return false;

        return (Mathf.Abs(v1.z * v1.z - v2.z * v2.z) <= 0.5f);
    }

    void ExportCorners(NavMeshPath path)
    {
        StreamWriter writer = new StreamWriter(filePath, true);

        bool first = true;
        foreach (Vector3 corner in path.corners)
        {
            if (first)
                first = false;
            else
                writer.WriteLine("C " + corner.x + " " + corner.z);
        }

        writer.Close();
    }
}
