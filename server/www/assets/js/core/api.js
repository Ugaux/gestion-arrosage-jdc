/************************************************/
/************** HTTP COMMUNICATION **************/
/************************************************/

export async function getSnapshot() {
  const res = await fetch("/api/snapshot");
  return res.json();
}
