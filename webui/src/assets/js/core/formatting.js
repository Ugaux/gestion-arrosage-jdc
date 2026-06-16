export function formatDuration(durationInSec) {
  const totalMinutes = Math.floor(durationInSec / 60);

  // < 1 minute → seconds
  if (totalMinutes === 0) {
    return `${durationInSec} s`;
  }

  const totalHours = Math.floor(durationInSec / 3600);
  const totalDays = Math.floor(durationInSec / 86400);

  const days = totalDays;
  const hours = totalHours % 24;
  const minutes = totalMinutes % 60;

  const parts = [];

  // 1m - 59m59s → minutes only
  if (totalHours === 0) parts.push(`${totalMinutes} min`);
  // 1h - 23h59m → hours + minutes
  else if (totalDays === 0) {
    parts.push(`${totalHours} h`);
    if (minutes > 0) {
      parts.push(`${minutes} min`);
    }
  }
  // 1d+ → days + hours
  else {
    parts.push(`${days} d`);
    if (hours > 0) {
      parts.push(`${hours} h`);
    }
  }

  return parts.join(" ");
}
