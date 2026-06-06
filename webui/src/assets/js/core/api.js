/************************************************/
/************** HTTP COMMUNICATION **************/
/************************************************/

export async function getSnapshot() {
  const res = await fetch("/api/snapshot");
  return res.json();
}

export async function getUsers() {
  const response = await fetch("/api/users");
  return response.json();
}
