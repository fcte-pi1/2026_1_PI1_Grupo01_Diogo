export default function RunPage({ params }: { params: { id: string } }) {
  return <h1>Corrida {params.id}</h1>;
}
